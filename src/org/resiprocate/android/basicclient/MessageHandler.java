package org.resiprocate.android.basicclient;

public interface MessageHandler {
	
	public void onMessage(String sender, String body);

}
