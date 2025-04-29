#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/CCHttpClient.hpp>
#include <alphalaneous.alphas_geode_utils/include/NodeModding.h>

using namespace geode::prelude;

class MyCCHttpRequest : public CCHttpRequest {
    public:
    void setProgress(int progress) {
        _downloadProgress = progress;
    }
    bool shouldCancel() {
        return _shouldCancel;
    }
};

class $objectModify(FieldsCCHttpRequest, CCHttpRequest) {
    struct Fields {
        std::shared_ptr<EventListener<web::WebTask>> m_downloadListener;
    };

    void modify() {}
};

class $modify(MyCCHttpClient, CCHttpClient) {

    void send(CCHttpRequest* request) {
        
        auto myRequest = static_cast<MyCCHttpRequest*>(request);
        auto fieldsRequest = reinterpret_cast<FieldsCCHttpRequest*>(request);
        auto requestFields = fieldsRequest->m_fields.self();
        auto req = web::WebRequest();

        auto start = reinterpret_cast<uint8_t*>(request->getRequestData());
        std::vector bytes(start, start + request->getRequestDataSize());
    
        if (!bytes.empty()){
            req.body(bytes);
        }

        req.userAgent("");
        req.version(web::HttpVersion::VERSION_2_0);

        requestFields->m_downloadListener = std::make_shared<EventListener<web::WebTask>>();

        Ref<CCHttpResponse> response = new CCHttpResponse(request);

        requestFields->m_downloadListener->bind([this, response, myRequest](web::WebTask::Event* e) {
            if (auto res = e->getValue()) {
                response->setSucceed(res->ok());
                response->setResponseCode(res->code());

                gd::vector<uint8_t> data = res->data();
                response->setResponseData(reinterpret_cast<gd::vector<char>*>(&data));
                
                SEL_HttpResponse pSelector = myRequest->getSelector();
                CCObject* pTarget = myRequest->getTarget();

                if (pTarget && pSelector) {
                    (pTarget->*pSelector)(this, response);
                }
                myRequest->release();
            }
            if (auto progress = e->getProgress()) {
                if (myRequest->shouldCancel()) {
                    e->cancel();
                }
                if (auto pr = progress->downloadProgress()) {
                    if (pr.has_value()) {
                        myRequest->setProgress(pr.value());
                    }
                }
            }
            if (e->isCancelled()) {
                myRequest->release();
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
        requestFields->m_downloadListener->setFilter(webtask);
    }
};