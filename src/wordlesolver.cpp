#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/beast.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/json.hpp>
#include <boost/system.hpp>

using namespace std;
using namespace boost;

string host = "www.nytimes.com";
string uri = "/svc/wordle/v2/";

asio::io_context io_ctx;


tm currentDate() {
    time_t now;
    time(&now);
    return *localtime(&now);
}

string dateToFilename(tm date) {
    stringstream ss;
    ss << date.tm_year + 1900;
    ss << "-";
    ss << setw(2) << setfill('0') << date.tm_mon + 1;
    ss << "-";
    ss << setw(2) << setfill('0') << date.tm_mday;
    ss << ".json";

    return ss.str();
}

string getSolution(tm date) {
    string filename = dateToFilename(date);

    asio::ssl::context ssl_ctx(asio::ssl::context::tlsv12);
    ssl_ctx.set_default_verify_paths();

    beast::ssl_stream<beast::tcp_stream> stream(io_ctx, ssl_ctx);
    stream.set_verify_mode(asio::ssl::verify_peer);
    stream.set_verify_callback(asio::ssl::rfc2818_verification(host));

    beast::get_lowest_layer(stream).connect(asio::ip::tcp::resolver(io_ctx).resolve(host, "443"));

    stream.handshake(asio::ssl::stream_base::client);

    beast::http::request<beast::http::string_body> req(beast::http::verb::get, uri + filename, 11);
    req.set(beast::http::field::host, host);
    req.set(beast::http::field::user_agent, "WordleSolver/0.1");

    beast::http::write(stream, req);

    beast::flat_buffer buffer;
    beast::http::response<beast::http::string_body> res;
    beast::http::read(stream, buffer, res);

    system::error_code ec;
    json::value json_data = json::parse(res.body(), ec);

    if (ec.failed()) return "???";

    return json_data.as_object()["solution"].as_string().c_str();
}

int main(int argc, char** argv) {
    string word = getSolution(currentDate());

    transform(word.begin(), word.end(), word.begin(), ::toupper);

    cout << word << endl;
}
