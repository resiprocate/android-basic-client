package org.resiprocate.android.basicclient;

public class MySipCallFactory implements SipCallFactory {

	public SipCall createSipCall(long basicClientCall, String peer) {
		return new MySipCall(basicClientCall, peer);
	}
	
}
