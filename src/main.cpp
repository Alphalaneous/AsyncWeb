#include <Geode/Geode.hpp>
#include <Geode/utils/web.hpp>
#include <Geode/modify/CCHttpClient.hpp>

using namespace geode::prelude;

std::map<CCHttpRequest*, EventListener<web::WebTask>*> m_downloadListeners;

class $modify(MyCCHttpClient, CCHttpClient){

	//here be dragons

	/*
		tldr; hooking the dtor didn't work, can't add fields to a CCObject
		somewhere in the chain, the reference count of the CCHttpRequest is fucked
		so I delete it, as I know it cannot be used anywhere else
		There has to be a global map that I can reference as well to prevent the EventListener from going out of scope
		and thus killing itself, normally I'd just add it as a field, but alas
	*/

	void send(CCHttpRequest* request){
    	auto req = web::WebRequest();

		std::vector<uint8_t> bytes;
		for(int i = 0; i < request->getRequestDataSize(); i++){
			bytes.push_back(static_cast<uint8_t>(request->getRequestData()[i]));
		}
	
		req.userAgent("");
		if(bytes.size() > 0){
			req.body(bytes);
		}
		req.version(web::HttpVersion::VERSION_2_0);

		EventListener<web::WebTask>* eventListener = new EventListener<web::WebTask>();

		request->retain();
		eventListener->bind([this, request](web::WebTask::Event* e){
        	if (auto res = e->getValue()){
				CCHttpResponse* oldResponse = new extension::CCHttpResponse(request);
        		oldResponse->setSucceed(res->ok());
				oldResponse->retain();

				if (res->ok()) {
					geode::Loader::get()->queueInMainThread([this, res, oldResponse, request](){
						auto data = res->data();

						gd::vector<char>* charData = new gd::vector<char>();
						for (int i = 0; i < data.size(); i++) {
							charData->push_back(data.at(i));
						}
						oldResponse->setResponseData(charData);
						delete charData;
						oldResponse->setResponseCode(res->code());

						SEL_HttpResponse pSelector = request->getSelector();
						CCObject* pTarget = request->getTarget();

						if (pTarget && pSelector) {
							(pTarget->*pSelector)(this, oldResponse);
						}

						oldResponse->release();
						request->release();
						
						EventListener<web::WebTask>* el = m_downloadListeners[request];
						m_downloadListeners.erase(m_downloadListeners.find(request));
						delete el;
						delete request;
					});
				}
				else{
					oldResponse->release();
					request->release();
					
					EventListener<web::WebTask>* el = m_downloadListeners[request];
					m_downloadListeners.erase(m_downloadListeners.find(request));
					delete el;
					delete request;
				}
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

		m_downloadListeners[request] = eventListener;
	}
};