package org.resiprocate.android.basiccall;

import java.util.logging.Logger;

public class MySipCall extends SipCall {

	Logger logger = Logger.getLogger(MySipCall.class.getCanonicalName());

	protected MySipCall(long basicClientCall, String peer) {
		super(basicClientCall, peer);
		logger.info("new MySipCall instance for peer: " + getPeer());
	}

	public void onProgress(int code, String reason) {
		logger.info("progress: " + code + ": " + reason);
	}

	public void onConnected() {
		logger.info("connected");
	}

	public void onFailure(int code, String reason) {
		logger.info("failure: " + code + ": " + reason);
	}

	public void onIncoming() {
		logger.info("onIncoming");
		answer();
	}

	public void onOfferRequired() {
		logger.info("onOfferRequired");
		// FIXME - call the WebRTC stack and set up an Observer to call provideOffer
		String testSdp = "v=0\r\n" +
                   "o=- 0 0 IN IP4 0.0.0.0\r\n" +
                   "s=basicAndroidClient\r\n" +
                   "c=IN IP4 0.0.0.0\r\n" +
                   "t=0 0\r\n" +
                   "m=audio 8000 RTP/AVP 0 101\r\n" +
                   "a=rtpmap:0 pcmu/8000\r\n" +
                   "a=rtpmap:101 telephone-event/8000\r\n" +
                   "a=fmtp:101 0-15\r\n";
		provideOffer(testSdp);
	}

	public void onAnswerRequired(String offer) {
		logger.info("onAnswerRequired");
		provideAnswer("");
	}
	
}
