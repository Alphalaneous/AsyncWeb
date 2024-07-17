#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/CCHttpClient.hpp>

using namespace geode::prelude;

std::map<CCHttpRequest*, EventListener<web::WebTask>*> m_downloadListeners;

class $modify(MyCCHttpClient, CCHttpClient) {

    //here be dragons

    void send(CCHttpRequest* request) {
        auto req = web::WebRequest();

        gd::vector<uint8_t> bytes;
        for (int i = 0; i < request->getRequestDataSize(); i++) {
            bytes.push_back(static_cast<uint8_t>(request->getRequestData()[i]));
        }
    
        if (bytes.size() > 0){
            req.body(bytes);
        }

        req.userAgent("");
        req.version(web::HttpVersion::VERSION_2_0);

        EventListener<web::WebTask>* eventListener = new EventListener<web::WebTask>();
        m_downloadListeners[request] = eventListener;

        CCHttpResponse* response = new CCHttpResponse(request);
        response->autorelease();

        eventListener->bind([this, request, eventListener, response](web::WebTask::Event* e) {
            if (auto res = e->getValue()) {
                Loader::get()->queueInMainThread([this, res, request, eventListener, response]() {
                    response->setSucceed(res->ok());

                    if (res->ok()) {
                        auto data = res->data();

                        gd::vector<char>* charData = new gd::vector<char>();
                        for (int i = 0; i < data.size(); i++) {
                            charData->push_back(data.at(i));
                        }
                        response->setResponseData(charData);
                        response->setResponseCode(res->code());

                        SEL_HttpResponse pSelector = request->getSelector();
                        CCObject* pTarget = request->getTarget();

                        if (pTarget && pSelector) {
                            (pTarget->*pSelector)(this, response);
                        }

                        delete charData;
                    }
                    
                    m_downloadListeners.erase(m_downloadListeners.find(request));
                    delete eventListener;
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