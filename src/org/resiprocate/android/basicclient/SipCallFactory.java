package org.resiprocate.android.basicclient;

public interface SipCallFactory {

	public SipCall createSipCall(long basicClientCall, String peer);
	
}
