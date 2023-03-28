#include <iostream>
#include <thread>

#include "AsioCoroutineUtil.h"
#include "Connection.h"
#include "IoContextPool.h"
#include "Lazy.h"
#include "SyncAwait.h"

using namespace boost::asio::ip;

class Server {
public:
    Server(IoContextPool& pool, unsigned short port)
        : _pool(pool), _port(port), _executor(pool.getIoContext()) {}

    Lazy<void> start() {
        tcp::acceptor acceptor(_pool.getIoContext(), tcp::endpoint(tcp::v4(), _port));
        while (true) {
            tcp::socket socket(_pool.getIoContext());
            if (auto err = co_await asyncAccept(acceptor, socket); err) {
                std::cerr << "Accept failed, error message: " << err.message() << std::endl;
                continue;
            }
            std::cout << "A HTTP request coming...\n";
            // Construct connection to handle request and respond.
            startOne(std::move(socket)).via(&_executor).detach();
        }
    }

    Lazy<void> startOne(tcp::socket socket) {
        Connection con(std::move(socket), "./");
        co_await con.start();
    }

private:
    IoContextPool& _pool;
    unsigned short _port;
    AsioExecutor _executor;
};

int main() {
    try {
        IoContextPool pool(10);
        std::thread t([&pool] { pool.run(); });
        Server server(pool, 2333);
        syncAwait(server.start());
        t.join();
    } catch (std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
    }
    return 0;
}
