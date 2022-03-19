#include <string>
#include <vector>


class Tachyon
{
public:
    Tachyon(const char *address, const char *port);
    ~Tachyon();
    double upload(const char *key, const char *filename, v8::Local<v8::Function> func, v8::Local<v8::Context> ctx, v8::Isolate *isolate);
    std::vector<std::string> list();

private:
    std::string m_UDPPort;
    std::string m_TCPPort;
    std::string m_Address;
};