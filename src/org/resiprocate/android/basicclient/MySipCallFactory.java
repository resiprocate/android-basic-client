package org.resiprocate.android.basicclient;

import android.os.RemoteCallbackList;

public class MySipCallFactory implements SipCallFactory {

	final RemoteCallbackList<SipUserRemote> mCallbacks;
	final SipService mSipService;

	public MySipCallFactory(RemoteCallbackList<SipUserRemote> callbacks, SipService s) {
		mCallbacks = callbacks;
		mSipService = s;
	}

	public SipCall createSipCall(long basicClientCall, String peer) {
		MySipCall call = new MySipCall(mCallbacks, basicClientCall, peer);
		mSipService.onNewSipCall(call);
		return call;
	}
	
}
