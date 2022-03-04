package org.resiprocate.android.basicclient;

interface SipUserRemote {

    void onMessage(String sender, String body);

    void onProgress(int code, String reason);

    void onConnected();

    void onFailure(int code, String reason);

    void onIncoming(String caller);

    void onTerminated(String reason);

    void onOfferRequired();

    void onAnswerRequired(String offer);

    void onAnswer(String answer);

}
