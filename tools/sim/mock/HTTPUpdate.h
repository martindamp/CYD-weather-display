#pragma once
#include "HTTPClient.h"
enum t_httpUpdate_return { HTTP_UPDATE_FAILED, HTTP_UPDATE_NO_UPDATES, HTTP_UPDATE_OK };
struct HTTPUpdateC { void rebootOnUpdate(bool) {} t_httpUpdate_return update(WiFiClientSecure&, const char*) { return HTTP_UPDATE_FAILED; } String getLastErrorString() { return "sim"; } };
extern HTTPUpdateC httpUpdate;
struct UpdateC { void onProgress(void(*)(int,int)) {} }; extern UpdateC Update;
