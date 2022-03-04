package org.resiprocate.android.basicclient;

import java.util.logging.Logger;

import android.os.RemoteCallbackList;
import android.os.RemoteException;

public class MySipCall extends SipCall {

	Logger logger = Logger.getLogger(MySipCall.class.getCanonicalName());

	final RemoteCallbackList<SipUserRemote> mCallbacks;

	protected MySipCall(RemoteCallbackList<SipUserRemote> callbacks, long basicClientCall, String peer) {
		super(basicClientCall, peer);
		mCallbacks = callbacks;
		logger.info("new MySipCall instance for peer: " + getPeer());
	}

	public void onProgress(int code, String reason) {
		logger.info("progress: " + code + ": " + reason);
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onProgress(code, reason);
			}
		}.run();
	}

	public void onConnected() {
		logger.info("connected");
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onConnected();
			}
		}.run();
	}

	public void onFailure(int code, String reason) {
		logger.info("failure: " + code + ": " + reason);
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onFailure(code, reason);
			}
		}.run();
	}

	public void onIncoming(String from) {
		logger.info("onIncoming");
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onIncoming(from);
			}
		}.run();
	}

	public void onTerminated(String reason) {
		logger.info("onTerminated");
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onTerminated(reason);
			}
		}.run();
	}

	public void onOfferRequired() {
		logger.info("onOfferRequired");
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onOfferRequired();
			}
		}.run();
	}

	public void onAnswerRequired(String offer) {
		logger.info("onAnswerRequired");
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onAnswerRequired(offer);
			}
		}.run();
	}

        public void onAnswer(String answer) {
                logger.info("onAnswer");
		new CallbackBroadcast() {
			@Override
			void execute(SipUserRemote u) throws RemoteException {
				u.onAnswer(answer);
			}
		}.run();
        }

	abstract class CallbackBroadcast {
		abstract void execute(SipUserRemote u) throws RemoteException;
		void run() {
			int callbackCount = mCallbacks.beginBroadcast();
			logger.info("callbackCount = " + callbackCount);
			for(int i = 0; i < callbackCount; i++) {
				try {
					logger.fine("invoking callback " + i);
					//mCallbacks.getBroadcastItem(i).onOfferRequired();
					execute(mCallbacks.getBroadcastItem(i));
				} catch (RemoteException ex) {
					logger.throwing("MySipCall", "onOfferRequired", ex);
				}
			}
			mCallbacks.finishBroadcast();
		}
	}
}
