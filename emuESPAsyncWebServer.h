
#pragma once

#include <WebSockets4WebServer.h>
#include <WebSocketsClient.h>
#include <Schedule.h>

#define EMUASYNC "EmuAsync: "

class AsyncWebSocket;
class AsyncWebSocketClient;
class AsyncWebServer;

typedef enum
{
    WS_EVT_CONNECT,
    WS_EVT_DISCONNECT,
    WS_EVT_PONG,
    WS_EVT_ERROR,
    WS_EVT_DATA
} AwsEventType;

typedef std::function<
    void(AsyncWebSocket * server,
         AsyncWebSocketClient * client,
         AwsEventType type,
         void * arg,
         uint8_t *data, size_t len)> AwsEventHandler;

class AsyncWebServer;
using AsyncWebHandler = AsyncWebServer;
using AsyncStaticWebHandler = AsyncWebServer;
using AsyncWebServerRequest = AsyncWebServer;
using AsyncCallbackWebHandler = AsyncWebServer;
using AsyncWebServerResponse = AsyncWebServer;
using AsyncWebSocketResponse = AsyncWebServer;
using AsyncResponseStream = AsyncWebServer;
using WebRequestMethodComposite = HTTPMethod;
using ArRequestHandlerFunction = std::function<void(AsyncWebServerRequest* request)>;
using AwsTemplateProcessor = std::function<String(const String&)>;

class AsyncWebParameter {
  private:
    String _name;
    String _value;
    size_t _size;
    bool _isForm;
    bool _isFile;

  public:

    AsyncWebParameter(const String& name, const String& value, bool form=false, bool file=false, size_t size=0): _name(name), _value(value), _size(size), _isForm(form), _isFile(file){}
    const String& name() const { return _name; }
    const String& value() const { return _value; }
    size_t size() const { return _size; }
    bool isPost() const { return _isForm; }
    bool isFile() const { return _isFile; }
};

class ESP8266WebServerDeleter: public ESP8266WebServer
{
private:

    void serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_header = NULL ) = delete;
    void on(const Uri &uri, THandlerFunction handler);
    void on(const Uri &uri, HTTPMethod method, THandlerFunction fn);
    void on(const Uri &uri, HTTPMethod method, THandlerFunction fn, THandlerFunction ufn);

public:
    ESP8266WebServerDeleter(int port): ESP8266WebServer(port)
    {
    }
};

class AsyncWebServer: public ESP8266WebServerDeleter
{
public:
    AsyncWebServer(int port): ESP8266WebServerDeleter(port)
    {
        if (!schedule_recurrent_function_us([&]()
        {
            this->handleClient();
            return true;
        }, 1000))
        {
            Serial.printf(EMUASYNC "can't register webserver recurrent sheduled function\n");
        }
    }

    void addHandler(AsyncWebSocket* wsServer);

    AsyncStaticWebHandler& serveStatic(const char* uri, fs::FS& fs, const char* path, const char* cache_control = nullptr)
    {
        ESP8266WebServer::serveStatic(uri, fs, path, cache_control);
        tempUri = uri;
        tempPath = path;
        tempFs = &fs;
        return *this;
    }

    // AsyncStaticWebHandler:
    AsyncStaticWebHandler& setDefaultFile(const char* filename)
    {
        ESP8266WebServer::serveStatic((tempUri + filename).c_str(), *tempFs, (tempPath + filename).c_str());
        return *this;
    }

    AsyncWebHandler& setAuthentication (const char *username, const char *password)
    {
        // todo
        (void)username;
        (void)password;
        return *this;
    }

    void onNotFound(ArRequestHandlerFunction onRequest)
    {
        ESP8266WebServer::onNotFound([&,onRequest]()
        {
            onRequest(this);
        });
    }

    AsyncCallbackWebHandler& on(const char* uri, WebRequestMethodComposite method, ArRequestHandlerFunction onRequest)
    {
        ESP8266WebServer::on(uri, method, [&,onRequest]()
        {
            onRequest(this);
        });
        return *this;
    }

    AsyncWebServerResponse* beginResponse_P(int code, const String& contentType, const uint8_t * content, size_t len, AwsTemplateProcessor callback = nullptr)
    {
        (void)callback;
        checkResponse();
        _respCode = code;
        _respContentType = contentType;
        _response = new StreamConstPtr(content, len);
        return this;
    }

    AsyncWebServerResponse* beginResponse_P(int code, const String& contentType, PGM_P content, AwsTemplateProcessor callback = nullptr)
    {
        (void)callback;
        checkResponse();
        _respCode = code;
        _respContentType = contentType;
        _response = new StreamConstPtr(content, strlen_P(content));
        return this;
    }

    AsyncResponseStream* beginResponseStream(const String& contentType)
    {
        checkResponse();
        _respCode = 200;
        _respContentType = contentType;
        _response = nullptr;
        return this;
    }

    void addHeader(const String& name, const String& value)
    {
        sendHeader(name, value);
    }

    void send(AsyncWebServerResponse* ignore)
    {
        (void)ignore;

        if (_response)
        {
#if 0
            chunkedResponseModeStart(_respCode, _respContentType);
            sendContent(_response);
            chunkedResponseFinalize();
#else
            ESP8266WebServer::send(_respCode, _respContentType.c_str(), _response);
#endif
            delete _response;
            _response = nullptr;
        }
        else
            Serial.println(EMUASYNC "no response in send(AsyncWebServerResponse)\n");
    }

    void send(int code, const char* content_type = "text/html", const String& content = emptyString)
    {
        return ESP8266WebServer::send(code, content_type, content);
    }

    void write(const char* buffer, size_t len)
    // used by ESPUI, after ::beginResponseStream()
    {
        return ESP8266WebServer::send(_respCode, _respContentType.c_str(), buffer, len);
    }

    bool hasParam(const String& name, bool post=false, bool file=false) const
    {
        (void)post;
        (void)file;
        return hasArg(name);
    }

    bool hasParam(const __FlashStringHelper * data, bool post=false, bool file = false) const
    {
        return hasParam(String(data), post, file);
    }

    AsyncWebParameter* getParam(const String& name, bool post = false, bool file = false) const
    {
        (void)post;
        (void)file;
        for (int i = 0; i < args(); i++)
            if (name == argName(i))
                return getParam(i);
        return nullptr;
    }

    AsyncWebParameter* getParam(const __FlashStringHelper * data, bool post, bool file) const
    {
        return getParam(String(data), post, file);
    }

    AsyncWebParameter* getParam(size_t num) const
    {
        (void)num;
        abort();
        return nullptr;
    }

    void redirect (const String& url)
    {
        sendHeader("Location", url, true);
        send(302);
    }

protected:

    void checkResponse ()
    {
        if (_response)
        {
            Serial.println(EMUASYNC "discard allocated but not send response!\n");
            delete _response;
            _response = nullptr;
        }
    }

    int _respCode;
    String _respContentType;
    Stream* _response = nullptr;

    String tempUri;
    String tempPath;
    fs::FS* tempFs;

    //vector<AsyncWebParameter> _params;
};


class AsyncWebSocketClient
{
public:

    void set (AsyncWebSocket* srv, WSclient_t* cli, uint32_t id)
    {
        _srv = srv;
        _cli = cli;
        _id = id;
    }

    uint32_t id () const { return _id; }
    void text(const String& t);
    bool canSend () { return true; }

    // compatibility with arduinoJson::serializeJson()
    size_t write (uint8_t c);
    size_t write (const uint8_t *buffer, size_t size);

protected:
    AsyncWebSocket* _srv = nullptr;
    WSclient_t* _cli = nullptr;
    uint32_t _id = 0;
};

class AsyncWebSocket: public WebSockets4WebServer
{
public:

    AsyncWebSocket(const char* root): _root(root)
    {
        if (!schedule_recurrent_function_us([&]()
        {
            this->loop();
            return true;
        }, 1000))
        {
            Serial.printf(EMUASYNC "can't register websocker server recurrent sheduled function\n");
        }
    }

    void addHandler(AsyncWebServer* webServer);

    /*AsyncWebHandler&*/void setAuthentication(const char *username, const char *password)
    {
        (void)username;
        (void)password;
        //TODO
    }

    size_t count ()
    {
        size_t ret = 0;
        for (int id = 0; id < WEBSOCKETS_SERVER_CLIENT_MAX; id++)
            if (clientIsConnected(&_clients[id]))
                ret++;
        return ret;
    }

    bool hasClient(int id)
    {
        return id < WEBSOCKETS_SERVER_CLIENT_MAX && clientIsConnected(&_clients[id]);
    }

    AsyncWebSocketClient* client(int id)
    {
        if (id < WEBSOCKETS_SERVER_CLIENT_MAX)
        {
            _clientsEmu[id].set(this, &_clients[id], id);
            return &_clientsEmu[id];
        }
        return nullptr;
    }

    void textAll(const String& t) { broadcastTXT(t.c_str(), t.length()); }

    void onEvent(AwsEventHandler handler) { _handler = handler; }

    void cleanupClients ()
    {
        WebSockets4WebServer::disconnect();
    }

private:

    String _root;
    AsyncWebSocketClient _clientsEmu[WEBSOCKETS_SERVER_CLIENT_MAX];
    AwsEventHandler _handler;

};
