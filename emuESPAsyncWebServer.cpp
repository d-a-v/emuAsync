
#include "emuESPAsyncWebServer.h"

void AsyncWebSocketClient::text(const String& t)
{
    _srv->sendTXT(_id, t.c_str(), t.length());
}

size_t AsyncWebSocketClient::write (uint8_t c)
{
    return _srv->sendTXT(_id, &c, 1);
}

size_t AsyncWebSocketClient::write (const uint8_t *buffer, size_t size)
{
    return _srv->sendTXT(_id, buffer, size);
}

void AsyncWebServer::addHandler(AsyncWebSocket* wsServer)
{
    wsServer->addHandler(this);
}

void AsyncWebSocket::addHandler(AsyncWebServer* webServer)
{
    webServer->addHook(hookForWebserver(_root.c_str(),
        [&](uint8_t num, WStype_t type, uint8_t * payload, size_t length)
        {
            /*
            void(AsyncWebSocket * server,
                 AsyncWebSocketClient * client,
                 AwsEventType type,
                 void * arg,
                 uint8_t *data, size_t len)> AwsEventHandler;
            */
            AwsEventType awsType;
            switch (type)
            {
            case WStype_ERROR:               awsType = WS_EVT_ERROR; break;
            case WStype_DISCONNECTED:        awsType = WS_EVT_DISCONNECT; break;
            case WStype_CONNECTED:           awsType = WS_EVT_CONNECT; break;
            case WStype_TEXT:
            case WStype_BIN:
            case WStype_FRAGMENT_TEXT_START:
            case WStype_FRAGMENT_BIN_START:
            case WStype_FRAGMENT:
            case WStype_FRAGMENT_FIN:        awsType = WS_EVT_DATA; break;
            case WStype_PING:
            case WStype_PONG:                awsType = WS_EVT_PONG; break;
            }
            _handler(this, client(num), awsType, nullptr, payload, length);
            
        }));
}
