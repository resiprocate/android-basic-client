package org.resiprocate.android.basiccall;

interface SipStackRemote {

    void sendMessage(String recipient, String body);

    void call(String recipient);
    
}
