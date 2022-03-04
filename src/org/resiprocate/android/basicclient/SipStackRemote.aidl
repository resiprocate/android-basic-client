package org.resiprocate.android.basicclient;

import org.resiprocate.android.basicclient.SipUserRemote;

interface SipStackRemote {

    void registerUser(SipUserRemote u);

    void sendMessage(String recipient, String body);

    void call(String recipient);

    void provideOffer(String offer);

    void provideAnswer(String answer);
    
}
