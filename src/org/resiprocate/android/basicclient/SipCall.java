package org.resiprocate.android.basicclient;

public abstract class SipCall {

	String mPeer;
	long mBasicClientCall = 0;
	protected SipCall(long basicClientCall, String peer) {
		mBasicClientCall = basicClientCall;
		mPeer = peer;
	}

	public String getPeer() {
		return mPeer;
	}

	public native void initiate();
	public native void cancel();
	public abstract void onProgress(int code, String reason);
	public abstract void onConnected();
	public abstract void onFailure(int code, String reason);

	public abstract void onIncoming();
	public native void reject();
	public native void answer();

	public abstract void onOfferRequired();
	public native void provideOffer(String offer);

	public abstract void onAnswerRequired(String offer);
	public native void provideAnswer(String answer);
	
}
