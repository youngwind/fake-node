#include "include/v8.h"
#include "include/libplatform/libplatform.h"
#include <assert.h>
#include <string.h>
#include <arpa/inet.h>
#include <errno.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <iostream>

#include <fstream>

using namespace std;

v8::Local<v8::Context> CreateShellContext(v8::Isolate *isolate);

void RunShell(v8::Local<v8::Context> context);

bool ExecuteString(v8::Isolate *isolate, v8::Local<v8::String> source);

void Console(const v8::FunctionCallbackInfo<v8::Value> &args);

void NewHttp(const v8::FunctionCallbackInfo<v8::Value> &args);

class ShellArrayBufferAllocator : public v8::ArrayBuffer::Allocator {
public:
    virtual void *Allocate(size_t length) {
        void *data = AllocateUninitialized(length);
        return data == NULL ? data : memset(data, 0, length);
    }

    virtual void *AllocateUninitialized(size_t length) { return malloc(length); }

    virtual void Free(void *data, size_t) { free(data); }
};

int main(int argc, char *argv[]) {
    // V8 初始化
    v8::V8::InitializeICU();
    v8::V8::InitializeExternalStartupData(argv[0]);
    v8::Platform *platform = v8::platform::CreateDefaultPlatform();
    v8::V8::InitializePlatform(platform);
    v8::V8::Initialize();

    ShellArrayBufferAllocator array_buffer_allocator;
    v8::Isolate::CreateParams create_params;
    create_params.array_buffer_allocator = &array_buffer_allocator;
    v8::Isolate *isolate = v8::Isolate::New(create_params);

    int result;
    {
        v8::Isolate::Scope isolate_scope(isolate);
        v8::HandleScope handle_scope(isolate);
        v8::Local<v8::Context> context = CreateShellContext(isolate);
        v8::Context::Scope context_scope(context);
        RunShell(context);
    }
    isolate->Dispose();
    v8::V8::Dispose();
    v8::V8::ShutdownPlatform();
    delete platform;
    return result;
}


v8::Local<v8::Context> CreateShellContext(v8::Isolate *isolate) {
    v8::Local<v8::ObjectTemplate> global = v8::ObjectTemplate::New(isolate);
    global->Set(
            v8::String::NewFromUtf8(isolate, "console", v8::NewStringType::kNormal)
                    .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, Console));
    global->Set(
            v8::String::NewFromUtf8(isolate, "newHttp", v8::NewStringType::kNormal)
                    .ToLocalChecked(),
            v8::FunctionTemplate::New(isolate, NewHttp));

    return v8::Context::New(isolate, NULL, global);
}

void Console(const v8::FunctionCallbackInfo<v8::Value> &args) {
    v8::String::Utf8Value str(args[0]);
    printf("%s", *str);
    printf("\n");
    fflush(stdout);
}

void NewHttp(const v8::FunctionCallbackInfo<v8::Value> &args) {
    int port = 80;
    string hostname = "www.baidu.com";
    int server;
    struct sockaddr_in server_addr;
    char buf[1024];
    string request;
    if (!args[0]->IsNull()) {
        v8::String::Utf8Value param1(args[0]->ToString());
        hostname = string(*param1);
    }
    if (!args[1]->IsNull()) {
        port = (int)(args[1]->IntegerValue());
    }

    request += "GET / ";
    request += "HTTP/1.1\r\n";
    request += "Host: ";
    request += hostname;
    request += "\r\n\r\n";   // 要注意最后的回车换行

    server = socket(PF_INET, SOCK_STREAM, 0);
    if (server < 0) {
        perror("socket");
        exit(-1);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    if (connect(server, (struct sockaddr *) &server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        exit(-1);
    }

    // 发送请求
    send(server, request.c_str(), request.length(), 0);

    // 接收返回
    recv(server, buf, 1024, 0);

    // 打印返回值
    cout << buf << endl;

    close(server);
}

void RunShell(v8::Local<v8::Context> context) {
    fprintf(stderr, "V8 version %s [sample shell]\n", v8::V8::GetVersion());
    static const int kBufferSize = 256;
    v8::Context::Scope context_scope(context);
    v8::Isolate *isolate = context->GetIsolate();
    while (true) {
        fprintf(stderr, "> ");
        char buffer[kBufferSize];
        char *str = fgets(buffer, kBufferSize, stdin);
        if (str == NULL) break;
        ExecuteString(isolate, v8::String::NewFromUtf8(isolate, str, v8::NewStringType::kNormal).ToLocalChecked());
    }
    fprintf(stderr, "\n");
}

bool ExecuteString(v8::Isolate *isolate, v8::Local<v8::String> source) {
    v8::HandleScope handle_scope(isolate);
    v8::Local<v8::Context> context(isolate->GetCurrentContext());
    v8::Local<v8::Script> script = v8::Script::Compile(context, source).ToLocalChecked();
    v8::Local<v8::Value> result = script->Run(context).ToLocalChecked();
    v8::String::Utf8Value str(result);
    printf("%s\n", *str);
    return true;
}
