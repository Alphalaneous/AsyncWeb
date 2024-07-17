#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/CCHttpClient.hpp>

using namespace geode::prelude;

/*

    After hours of trying, something for some reason makes this mod not work on android. 
    I have tried everything, to no avail. This mod was not destined for an android release
    for android is the strongest form of evil known to man. There will likely never be android
    support unless an angel pops in and resolves all the horrors within this small but powerful
    mod here. I hope that anyone that tries to resolve the issues here can do so peacefully,
    unlike me, who is ready to explode. 

    I sit here in my chair, staring at the code, wondering what even brought me here. It started
    back in the earlier days of Geometry Dash modding. Back in 2.1, I made a mod that ported Axmol's
    HttpClient to the game, which allowed asynchronous requests to work. This mod was called
    ConcurrentHTTP. I thought porting this to 2.2 would be easy, and that there wouldn't be trouble,
    but oh man was I wrong. 
    
    This mod has led to me realizing that quite a bit of people in the Geode discord server can be a 
    bit dickish. People tend to say my code is bad without ever telling me what it is exactly or why
    it is bad. It's always in indirect comments that are almost exactly describing me. When I bring 
    up that I want to know what I am doing wrong and why my code is "Shit", I'm hit with silence or
    being told they don't want to tell me. I do not know how to improve if the people around me
    refuse to tell me what I am doing wrong. 

    If you are reading this right now, I don't know how to code, everything you see is just be knowing
    logic and piecing things together. I do not retain actual information, just syntax and order. 
    Most of my projects are hours of Googling solutions that I feel will work together for a full
    project, but alas I am met with the realization that nothing I make is good, nor worthwhile in
    the end, and that it is dogshit compared to real programmers. 

*/

static std::map<CCHttpRequest*, std::shared_ptr<EventListener<web::WebTask>>> m_downloadListeners;

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

        std::shared_ptr<EventListener<web::WebTask>> eventListener = std::make_shared<EventListener<web::WebTask>>();
        m_downloadListeners[request] = eventListener;

        CCHttpResponse* response = new CCHttpResponse(request);
        response->autorelease();

        eventListener->bind([this, request, eventListener, response](web::WebTask::Event* e) {
            if (auto res = e->getValue()) {
                Loader::get()->queueInMainThread([this, res, request, eventListener, response]() {
                    response->setSucceed(res->ok());

                    if (res->ok()) {
                        auto data = res->data();
                        
                        gd::vector<char> charData;
                        std::copy(data.begin(), data.end(), std::back_inserter(charData));

                        response->setResponseData(&charData);
                        response->setResponseCode(res->code());

                        SEL_HttpResponse pSelector = request->getSelector();
                        CCObject* pTarget = request->getTarget();

                        if (pTarget && pSelector) {
                            (pTarget->*pSelector)(this, response);
                        }
                    }

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