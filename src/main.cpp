#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/CCHttpClient.hpp>
#include <Geode/modify/MusicDownloadManager.hpp>

using namespace geode::prelude;

/*

    @sleepyut is based, thanks for helping fix this mess to work for android!!

*/

static std::map<CCHttpRequest*, std::shared_ptr<EventListener<web::WebTask>>> m_downloadListeners;

class $modify(MyCCHttpClient, CCHttpClient) {

    //here be dragons

    void send(CCHttpRequest* request) {

        std::string url = std::string(request->getUrl());

        log::info("URL: {}", url);

        if(url.ends_with(".php") && url.find("backup") != std::string::npos){
            log::info("HERE!");
            CCHttpClient::send(request);
            return;
        }

        auto req = web::WebRequest();

        auto start = reinterpret_cast<uint8_t*>(request->getRequestData());
        std::vector bytes(start, start + request->getRequestDataSize());
    
        if (!bytes.empty()){
            req.body(bytes);
        }

        req.userAgent("");
        req.version(web::HttpVersion::VERSION_2_0);

        std::shared_ptr<EventListener<web::WebTask>> eventListener = std::make_shared<EventListener<web::WebTask>>();
        m_downloadListeners[request] = eventListener;

        CCHttpResponse* response = new CCHttpResponse(request);
        response->retain();

        eventListener->bind([this, request, eventListener, response](web::WebTask::Event* e) {
            if (auto res = e->getValue()) {
                Loader::get()->queueInMainThread([this, res, request, eventListener, response]() {
                    response->setSucceed(res->ok());

                    if (res->ok()) {
                        
                        gd::vector<uint8_t> data = res->data();

                        response->setResponseData(reinterpret_cast<gd::vector<char>*>(&data));
                        response->setResponseCode(res->code());

                        SEL_HttpResponse pSelector = request->getSelector();
                        CCObject* pTarget = request->getTarget();

                        if (pTarget && pSelector) {
                            (pTarget->*pSelector)(this, response);
                        }
                    }

                    response->release();
                    m_downloadListeners.erase(m_downloadListeners.find(request));
                });
            }
        });

        web::WebTask webtask;

        switch (request->getRequestType()) {
            case CCHttpRequest::kHttpGet:
                webtask = req.get(request->getUrl());
                break;
            case CCHttpRequest::kHttpPost:
                webtask = req.post(request->getUrl());
                break;
            case CCHttpRequest::kHttpPut:
                webtask = req.put(request->getUrl());
                break;
            default:
                webtask = req.post(request->getUrl());
        }
        eventListener->setFilter(webtask);

    }
};