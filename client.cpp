
#include <iostream>
#include <string>
#include <boost/asio.hpp>
#include <sqlite3.h>

using boost::asio::ip::tcp;

class ChatServer {
public:
    ChatServer(boost::asio::io_context& io_context, short port)
        : acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
        socket_(io_context) {
        InitDatabase();
        StartAccept();
    }

private:
    void InitDatabase() {
        sqlite3_open("chat.db", &db_);

        // —оздаем таблицу дл€ хранени€ сообщений
        std::string create_table_query =
            "CREATE TABLE IF NOT EXISTS messages ("
            "id INTEGER PRIMARY KEY AUTOINCREMENT,"
            "username TEXT,"
            "message TEXT);";

        sqlite3_exec(db_, create_table_query.c_str(), 0, 0, 0);
    }

    void StartAccept() {
        acceptor_.async_accept(
            socket_,
            [this](boost::system::error_code ec) {
                if (!ec) {
                    std::make_shared<ChatSession>(std::move(socket_), db_)->Start();
                }

                StartAccept();
            });
    }

    tcp::acceptor acceptor_;
    tcp::socket socket_;
    sqlite3* db_;
};

class ChatSession : public std::enable_shared_from_this<ChatSession> {
public:
    ChatSession(tcp::socket socket, sqlite3* db)
        : socket_(std::move(socket)),
        db_(db) {}

    void Start() {
        DoRead();
    }

private:
    void DoRead() {
        auto self(shared_from_this());
        socket_.async_read_some(
            boost::asio::buffer(data_, max_length),
            [this, self](boost::system::error_code ec, std::size_t length) {
                if (!ec) {
                    // ќбработка прочитанных данных (сохранение в базу данных и т.д.)
                    DoWrite(length);
                }
            });
    }

    void DoWrite(std::size_t length) {
        auto self(shared_from_this());
        socket_.async_write_some(
            boost::asio::buffer(data_, length),
            [this, self](boost::system::error_code ec, std::size_t /*length*/) {
                if (!ec) {
                    DoRead();
                }
            });
    }

    tcp::socket socket_;
    sqlite3* db_;
    enum { max_length = 1024 };
    char data_[max_length];
};

int main() {
    try {
        boost::asio::io_context io_context;
        ChatServer server(io_context, 12345);
        io_context.run();
    }
    catch (std::exception& e) {
        std::cerr << e.what() << std::endl;
    }

    return 0;
}