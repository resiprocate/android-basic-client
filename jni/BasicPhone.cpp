
#include "rutil/AndroidLogger.hxx"
#include "rutil/Log.hxx"
#include "rutil/Logger.hxx"
#include "rutil/Subsystem.hxx"
#include "resip/dum/ClientAuthManager.hxx"
#include "resip/dum/ClientRegistration.hxx"
#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/DumShutdownHandler.hxx"
#include "resip/dum/InviteSessionHandler.hxx"
#include "resip/dum/MasterProfile.hxx"
#include "resip/dum/Profile.hxx"
#include "resip/dum/UserProfile.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/ClientPagerMessage.hxx"
#include "resip/dum/ServerPagerMessage.hxx"

#include "resip/dum/DialogUsageManager.hxx"
#include "resip/dum/AppDialogSet.hxx"
#include "resip/dum/AppDialog.hxx"
#include "resip/dum/RegistrationHandler.hxx"
#include "resip/dum/PagerMessageHandler.hxx"
#include "resip/stack/PlainContents.hxx"
#include "resip/stack/ShutdownMessage.hxx"

#include <iostream>
#include <string>
#include <sstream>

#include <jni.h>

#include "basicClientUserAgent.hxx"
#include "basicClientCall.hxx"

using namespace std;
using namespace resip;

#define RESIPROCATE_SUBSYSTEM Subsystem::TEST

#define DEFAULT_REGISTRATION_EXPIRY 600

#define DEFAULT_UDP_PORT 5068
#define DEFAULT_TCP_PORT 5068
#define DEFAULT_TLS_PORT 5069

static SipStack *clientStack;
static DialogUsageManager *clientDum;
static std::shared_ptr<BasicClientUserAgent> basicUA;

class RegListener : public ClientRegistrationHandler {
public:
        RegListener() : _registered(false) {};
        bool isRegistered() { return _registered; };

        virtual void onSuccess(ClientRegistrationHandle, const SipMessage& response)
    {
        cout << "client registered\n";
            _registered = true;
    }
        virtual void onRemoved(ClientRegistrationHandle, const SipMessage& response)
    {
        cout << "client regListener::onRemoved\n";
            exit(-1);
    }
        virtual void onFailure(ClientRegistrationHandle, const SipMessage& response)
    {
        cout << "client regListener::onFailure\n";
            exit(-1);
    }
    virtual int onRequestRetry(ClientRegistrationHandle, int retrySeconds, const SipMessage& response)
    {
        cout << "client regListener::onRequestRetry\n";
            exit(-1);
        return -1;
    }

protected:
        bool _registered;

};


class ClientMessageHandler : public ClientPagerMessageHandler {
public:
   ClientMessageHandler()
      : finished(false),
        successful(false)
   {
   };

   virtual void onSuccess(ClientPagerMessageHandle, const SipMessage& status)
   {
      InfoLog(<<"ClientMessageHandler::onSuccess\n");
      successful = true;
      finished = true;
   }

   virtual void onFailure(ClientPagerMessageHandle, const SipMessage& status, std::unique_ptr<Contents> contents)
   {
      ErrLog(<<"ClientMessageHandler::onFailure\n");
      successful = false;
      finished = true;
   }

   bool isFinished() { return finished; };
   bool isSuccessful() { return successful; };

private:
   bool finished;
   bool successful;
};

static JNIEnv *_env;
static jobject message_handler = 0;
static jmethodID on_message_method = 0;

static jobject sip_call_factory = 0;
static jmethodID create_sip_call_method = 0;

class ServerMessageHandler : public ServerPagerMessageHandler
{
public:
        ServerMessageHandler() {};
        virtual void onMessageArrived(ServerPagerMessageHandle handle, const SipMessage& message)
    {

            std::shared_ptr<SipMessage> ok = handle->accept();
            handle->send(ok);

            Contents *body = message.getContents();
            InfoLog(<< "Got a message: " << *body);

            if(message_handler == 0 || on_message_method == 0)
            {
               ErrLog(<< "No MessageHandler has been set, ignoring message");
               return;
            }
            
            jstring _body = _env->NewStringUTF(body->getBodyData().c_str());
            jstring _sender = _env->NewStringUTF(message.header(h_From).uri().getAor().c_str());
            
            _env->CallObjectMethod(message_handler, on_message_method, _sender, _body);

            _env->DeleteLocalRef(_sender);
            _env->DeleteLocalRef(_body);
            
    }
};


// A proper implementation must keep the SIP stack active as long as the
// app is running.

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipStack_call
  (JNIEnv *env, jobject jthis, jstring recipient)
{
   const char *_recipient = env->GetStringUTFChars(recipient, 0);

   try {

      InfoLog(<< "Making call to " << _recipient);

      BasicClientCall* _call = new BasicClientCall(*basicUA);

      DebugLog(<< "sip_call_factory: " << sip_call_factory << " create_sip_call_method: " << create_sip_call_method);
      jobject _sip_call = env->CallObjectMethod(sip_call_factory, create_sip_call_method, (jlong)_call, recipient);
      _call->setupJni(env, jthis, _sip_call);
      jclass objclass = env->GetObjectClass(_sip_call);

      jmethodID on_offer_required_method = env->GetMethodID(objclass, "onOfferRequired", "()V");
      if(on_offer_required_method == 0)
      {
         ErrLog(<<"failed to get onOfferRequired");
         return;
      }
      InfoLog(<< "calling onOfferRequired");
      env->CallVoidMethod(_sip_call, on_offer_required_method);
   }
   catch (exception& e)
   {
      ErrLog(<< e.what());
   }
   catch(...)
   {
      ErrLog(<< "some exception!");
   }

   env->ReleaseStringUTFChars(recipient, _recipient);
}

JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipStack_sendMessage
  (JNIEnv *env, jobject jthis, jstring recipient, jstring body)
{
   const char *_recipient = env->GetStringUTFChars(recipient, 0);
   const char *_body = env->GetStringUTFChars(body, 0);
   
   try {

      InfoLog(<< "Sending MESSAGE\n");
      NameAddr naTo(_recipient);
      ClientPagerMessageHandle cpmh = clientDum->makePagerMessage(naTo);

      Data messageBody(_body);
      std::unique_ptr<Contents> content(new PlainContents(messageBody));
      cpmh.get()->page(std::move(content));

   }
   catch (exception& e)
   {
      cout << e.what() << endl;
   }
   catch(...)
   {
      cout << "some exception!" << endl;
   }
    
   env->ReleaseStringUTFChars(recipient, _recipient);
   env->ReleaseStringUTFChars(body, _body);
}

/*
 * Class:     org_resiprocate_android_basicclient_SipStack
 * Method:    init
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipStack_init
  (JNIEnv *env, jobject jthis, jstring sipUser, jstring realm, jstring user, jstring password)
{
   const char *_sipUser = env->GetStringUTFChars(sipUser, 0);
   const char *_realm = env->GetStringUTFChars(realm, 0);
   const char *_user = env->GetStringUTFChars(user, 0);
   const char *_password = env->GetStringUTFChars(password, 0);

   AndroidLogger *alog = new AndroidLogger();
   Log::initialize(Log::Cout, Log::Stack, "SIP", *alog);
   
   /*
   RegListener client;
   std::shared_ptr<MasterProfile> profile(new MasterProfile);
   std::unique_ptr<ClientAuthManager> clientAuth(new ClientAuthManager());

   clientStack = new SipStack();
   // stats service creates timers requiring extra wakeups, so we disable it
   clientStack->statisticsManagerEnabled() = false;
   clientDum = new DialogUsageManager(*clientStack);

   // Enable all the common transports:
   clientStack->addTransport(UDP, DEFAULT_UDP_PORT);
   //clientStack->addTransport(TCP, DEFAULT_TCP_PORT);
   //clientStack->addTransport(TLS, DEFAULT_TLS_PORT);

   clientDum->setMasterProfile(profile);
   
   clientDum->setClientRegistrationHandler(&client);

   clientDum->setClientAuthManager(std::move(clientAuth));
   clientDum->getMasterProfile()->setDefaultRegistrationTime(DEFAULT_REGISTRATION_EXPIRY);
   clientDum->getMasterProfile()->addSupportedMethod(MESSAGE);
   clientDum->getMasterProfile()->addSupportedMimeType(MESSAGE, Mime("text", "plain"));
   
   ClientMessageHandler *cmh = new ClientMessageHandler();
   clientDum->setClientPagerMessageHandler(cmh);
   
   ServerMessageHandler *smh = new ServerMessageHandler();
   clientDum->setServerPagerMessageHandler(smh);
   
   NameAddr naFrom(_sipUser);
   profile->setDefaultFrom(naFrom);
   profile->setDigestCredential(_realm, _user, _password);
   
   std::shared_ptr<SipMessage> regMessage = clientDum->makeRegistration(naFrom);
   clientDum->send( regMessage );
   */

   static int argc = 3;
   static char *argv[] = { "BasicClient", "--aor", strdup(_sipUser), 0 };
   InfoLog(<<"construct BasicClientUserAgent");
   basicUA.reset(new BasicClientUserAgent(argc, argv));
   InfoLog(<<"start BasicClientUserAgent");
   basicUA->startup();
   
   env->ReleaseStringUTFChars(sipUser, _sipUser);
   env->ReleaseStringUTFChars(realm, _realm);
   env->ReleaseStringUTFChars(user, _user);
   env->ReleaseStringUTFChars(password, _password);

}

/*
 * Class:     org_resiprocate_android_basicclient_SipStack
 * Method:    handleEvents
 * Signature: ()V
 */
JNIEXPORT jlong JNICALL Java_org_resiprocate_android_basicclient_SipStack_handleEvents
  (JNIEnv *env, jobject)
{
   // This is used by callbacks:
   _env = env;

   /*
   clientStack->process(10);
   while(clientDum->process());
   return clientStack->getTimeTillNextProcessMS();
   */
   //StackLog(<<"BasicClientUserAgent event loop");
   try {
      basicUA->process(3);
   } catch(...) {
      ErrLog(<<"unhandled exception");
   }
   //StackLog(<<"BasicClientUserAgent event loop done");
   return 10; // FIXME
}

class MyShutdownHandler : public DumShutdownHandler
{
   public:
      MyShutdownHandler() : mShutdown(false) { }
      virtual ~MyShutdownHandler(){} 

      bool isShutdown() { return mShutdown; }
      virtual void onDumCanBeDeleted() 
      {
         InfoLog( << "onDumCanBeDeleted was called");
         mShutdown = true;
      }
   private:
      volatile bool mShutdown;
};

/*
 * Class:     org_resiprocate_android_basicclient_SipStack
 * Method:    done
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipStack_done
  (JNIEnv *env, jobject)
{
   // May be used in some callbacks during shutdown:
   _env = env;

   MyShutdownHandler mDumShutdown;
   clientDum->shutdown(&mDumShutdown);
   while(!mDumShutdown.isShutdown())
   {
      clientStack->process(10);
      while(clientDum->process());
   }
   delete clientDum;

   InfoLog(<<"DUM shutdown OK, now for the stack");

   clientStack->shutdown();
   ShutdownMessage *shutdownFinished = 0;
   while(shutdownFinished == 0)
   {
      clientStack->process(10);
      Message *msg = clientStack->receiveAny();
      shutdownFinished = dynamic_cast<ShutdownMessage*>(msg);
      delete msg;
   }

   delete clientStack;
   InfoLog(<<"Stack shutdown OK");

   // Remove these last, they may be called during shutdown
   if(message_handler != 0)
   {
      env->DeleteGlobalRef(message_handler);
   }
}

/*
 * Class:     org_resiprocate_android_basicclient_SipStack
 * Method:    setSipCallFactory
 * Signature: (Lorg/resiprocate/android/basicclient/SipCallFactory;)V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipStack_setSipCallFactory
  (JNIEnv *env, jobject _this, jobject _sip_call_factory)
{
   sip_call_factory = env->NewGlobalRef(_sip_call_factory);
   jclass objclass = env->GetObjectClass(sip_call_factory);
   create_sip_call_method = env->GetMethodID(objclass, "createSipCall", "(JLjava/lang/String;)Lorg/resiprocate/android/basicclient/SipCall;");
   if(create_sip_call_method == 0){
      ErrLog( << "could not get method id\n");
      return;
   }
}

/*
 * Class:     org_resiprocate_android_basicclient_SipStack
 * Method:    setMessageHandler
 * Signature: (Lorg/resiprocate/android/basicclient/MessageHandler;)V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipStack_setMessageHandler
  (JNIEnv *env, jobject _this, jobject _message_handler)
{
   message_handler = env->NewGlobalRef(_message_handler);
   jclass objclass = env->GetObjectClass(_message_handler);
   on_message_method = env->GetMethodID(objclass, "onMessage", "(Ljava/lang/String;Ljava/lang/String;)V");
      if(on_message_method == 0){
          ErrLog( << "could not get method id!\n");
          return;
      }
      
   
}


/*
 * Class:     org_resiprocate_android_basicclient_SipCall
 * Method:    initiate
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipCall_initiate
  (JNIEnv *env, jobject _this)
{
   jclass _class = env->GetObjectClass(_this);
   jfieldID mPeerFID = env->GetFieldID(_class, "mPeer", "Ljava/lang/String;");
   jstring mPeer = (jstring)env->GetObjectField(_this, mPeerFID);
   const char *_mPeer = env->GetStringUTFChars(mPeer, 0);
   jfieldID mBasicClientCallFID = env->GetFieldID(_class, "mBasicClientCall", "J");
   jlong mBasicClientCall = env->GetLongField(_this, mBasicClientCallFID);
   BasicClientCall* _call = (BasicClientCall*)mBasicClientCall;

   if(_call != nullptr)
   {
      ErrLog( << "call already in progress");
      return;
   }
   _call = new BasicClientCall(*basicUA);
   _call->setupJni(env, 0, _this); // FIXME - sipStack
   NameAddr target(_mPeer);
   _call->initiateCall(target.uri(), basicUA->getUserProfile());
   mBasicClientCall = (jlong)_call;

   env->ReleaseStringUTFChars(mPeer, _mPeer);
}

/*
 * Class:     org_resiprocate_android_basicclient_SipCall
 * Method:    cancel
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipCall_cancel
  (JNIEnv *env, jobject _this)
{
   InfoLog(<<"cancel() invoked");

   jclass objclass = env->GetObjectClass(_this);
   jfieldID mBasicClientCallFID = env->GetFieldID(objclass, "mBasicClientCall", "J");
   BasicClientCall* _call = (BasicClientCall*)env->GetLongField(_this, mBasicClientCallFID);

   _call->endCommand();
}

/*
 * Class:     org_resiprocate_android_basicclient_SipCall
 * Method:    reject
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipCall_reject
  (JNIEnv *env, jobject _this)
{
   InfoLog(<<"reject() invoked");

   jclass objclass = env->GetObjectClass(_this);
   jfieldID mBasicClientCallFID = env->GetFieldID(objclass, "mBasicClientCall", "J");
   BasicClientCall* _call = (BasicClientCall*)env->GetLongField(_this, mBasicClientCallFID);

   _call->endCommand();
}

/*
 * Class:     org_resiprocate_android_basicclient_SipCall
 * Method:    answer
 * Signature: ()V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipCall_answer
  (JNIEnv *env, jobject _this)
{
   InfoLog(<<"answer() invoked");
}

/*
 * Class:     org_resiprocate_android_basicclient_SipCall
 * Method:    provideOffer
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipCall_provideOffer
  (JNIEnv *env, jobject _this, jstring offer)
{
   const char *_offer = env->GetStringUTFChars(offer, 0);
   Data localSdp(_offer);
   env->ReleaseStringUTFChars(offer, _offer);
   InfoLog(<<"received the offer from Java: " << localSdp);

   jclass _class = env->GetObjectClass(_this);
   jfieldID mPeerFID = env->GetFieldID(_class, "mPeer", "Ljava/lang/String;");
   jstring mPeer = (jstring)env->GetObjectField(_this, mPeerFID);
   const char *_mPeer = env->GetStringUTFChars(mPeer, 0);
   jfieldID mBasicClientCallFID = env->GetFieldID(_class, "mBasicClientCall", "J");
   BasicClientCall* _call = (BasicClientCall*)env->GetLongField(_this, mBasicClientCallFID);

   NameAddr target(_mPeer);
   env->ReleaseStringUTFChars(mPeer, _mPeer);
   // FIXME - only start a call on first offer
   _call->setLocalSdp(localSdp);
   _call->initiateCall(target.uri(), basicUA->getUserProfile());
}

/*
 * Class:     org_resiprocate_android_basicclient_SipCall
 * Method:    provideAnswer
 * Signature: (Ljava/lang/String;)V
 */
JNIEXPORT void JNICALL Java_org_resiprocate_android_basicclient_SipCall_provideAnswer
  (JNIEnv *env, jobject _this, jstring answer)
{
   const char *_answer = env->GetStringUTFChars(answer, 0);
   Data localSdp(_answer);
   env->ReleaseStringUTFChars(answer, _answer);
   InfoLog(<<"received the answer from Java: " << localSdp);

   jclass _class = env->GetObjectClass(_this);
   jfieldID mBasicClientCallFID = env->GetFieldID(_class, "mBasicClientCall", "J");
   BasicClientCall* _call = (BasicClientCall*)env->GetLongField(_this, mBasicClientCallFID);

   // FIXME - give the SDP answer to the BasicClientCall
   //       - call accept if necessary
   _call->setLocalSdp(localSdp);
}

#ifdef __cplusplus
}
#endif
