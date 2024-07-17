#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/CCHttpClient.hpp>

using namespace geode::prelude;

std::map<CCHttpRequest*, std::shared_ptr<EventListener<web::WebTask>>> m_downloadListeners;

class $modify(MyCCHttpClient, CCHttpClient) {

    //here be dragons

    void send(CCHttpRequest* request) {
        auto req = web::WebRequest();

        std::vector<uint8_t> bytes;
        for (int i = 0; i < request->getRequestDataSize(); i++) {
            bytes.push_back(static_cast<uint8_t>(request->getRequestData()[i]));
        }
    
        if (bytes.size() > 0){
            req.body(bytes);
        }

        req.userAgent("");
        req.version(web::HttpVersion::VERSION_2_0);

        log::info("got here 1");

        std::shared_ptr<EventListener<web::WebTask>> eventListener = std::make_shared<EventListener<web::WebTask>>();
        m_downloadListeners[request] = eventListener;

        CCHttpResponse* response = new CCHttpResponse(request);
        response->autorelease();

        log::info("got here 2");

        eventListener->bind([this, request, eventListener, response](web::WebTask::Event* e) {
            if (auto res = e->getValue()) {
                Loader::get()->queueInMainThread([this, res, request, eventListener, response]() {
                    response->setSucceed(res->ok());

                    if (res->ok()) {
                        auto data = res->data();
                        
                        std::shared_ptr<gd::vector<char>> charData = std::make_shared<gd::vector<char>>();
                        std::copy(data.begin(), data.end(), std::back_inserter(*charData));

                        log::info("got here 3");

                        response->setResponseData(charData.get());
                        response->setResponseCode(res->code());

                        SEL_HttpResponse pSelector = request->getSelector();
                        CCObject* pTarget = request->getTarget();

                        if (pTarget && pSelector) {
                            (pTarget->*pSelector)(this, response);
                        }

                        log::info("got here 4");

                        log::info("got here 5");
                    }
                    
                    m_downloadListeners.erase(m_downloadListeners.find(request));

                    log::info("got here 6");
                });
            }
        });

        log::info("got here 7");

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

        log::info("got here 8");

        eventListener->setFilter(webtask);

        log::info("got here 9");
    }
};