package org.resiprocate.android.basicclient;

interface SipStackRemote {

    void sendMessage(String recipient, String body);

    void call(String recipient);
    
}
