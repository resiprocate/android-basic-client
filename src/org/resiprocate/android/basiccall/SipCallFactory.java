package org.resiprocate.android.basiccall;

public interface SipCallFactory {

	public SipCall createSipCall(long basicClientCall, String peer);
	
}
