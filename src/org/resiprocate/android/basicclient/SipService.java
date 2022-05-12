package org.resiprocate.android.basicclient;

import java.util.Timer;
import java.util.TimerTask;
import java.util.logging.Logger;

import android.app.AlarmManager;
import android.app.Notification;
import android.app.NotificationManager;
import android.app.PendingIntent;
import android.app.Service;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.content.SharedPreferences;
import android.media.RingtoneManager;
import android.net.Uri;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.IBinder;
import android.os.Looper;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.os.RemoteCallbackList;
import android.os.RemoteException;
import android.os.SystemClock;
import android.preference.PreferenceManager;

public class SipService extends Service {
	
	Logger logger = Logger.getLogger(SipService.class.getCanonicalName());

	String mSipAddr;
	String mSipRealm;
	String mSipUser;
	String mSipPassword;

	Handler mHandler;

	final RemoteCallbackList<SipUserRemote> mCallbacks = new RemoteCallbackList<SipUserRemote>();

	public SipService() {
		System.loadLibrary("c++_shared");
		System.loadLibrary("crypto");
		System.loadLibrary("ssl");
		System.loadLibrary("resipares");
		System.loadLibrary("rutil");
		System.loadLibrary("resip");
		System.loadLibrary("dum");
		System.loadLibrary("BasicPhone");
	}

	final static String TAG = "SipService";
	private static final long INITIAL_DELAY = 50L;
	private static final long STACK_INTERVAL = 10L;

	private static final String ALARM_ACTION = "org.resiprocate.android.basicclient.SipService.WAKEUP";
	
	AlarmManager alarmManager;
	private PendingIntent pendingIntent;
	private PendingIntent pendingIntentWakeup;

	SipStack mSipStack = null;

	SipCall mSipCall = null;

	private void setAlarms(long delay) {
		// https://developer.android.com/reference/android/app/AlarmManager#set(int,%20long,%20android.app.PendingIntent)
		// set(..) does not provide an exact delay since API level 19
		// setExact(..) requires extra permissions since API level 31
		//alarmManager.set( AlarmManager.ELAPSED_REALTIME, SystemClock.elapsedRealtime() + STACK_INTERVAL, pendingIntent );
		// Make sure we get woken up again if the phone sleeps:
		//alarmManager.set( AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + delay, pendingIntentWakeup );
		mHandler.postDelayed(new Runnable() {
			public void run() {
				runEventLoop();
			}
		}, delay);
	}
	
	private void cancelAlarms() {
		alarmManager.cancel(pendingIntent);
		alarmManager.cancel(pendingIntentWakeup);
	}
	
	private void runEventLoop() {
		cancelAlarms();
		logger.finest("trying to run event loop");
		long delay = 0;
		while(delay == 0)
			synchronized(mSipStack) {
				delay = mSipStack.handleEvents();
			}
		logger.finest("finished running event loop, next delay = " + delay + " ms");
		setAlarms(delay);
	}

	@Override
	public IBinder onBind(Intent arg0) {
		return mBinder;
	}
	
	private void configure() {
	    SharedPreferences sp= PreferenceManager.getDefaultSharedPreferences(this);
	    mSipAddr = sp.getString("uri", "anonymous@example.org");
	    mSipRealm = sp.getString("realm", "example.org");
	    mSipUser = sp.getString("user", "anonymous");
	    mSipPassword = sp.getString("password", "123");
	}

	@Override
	public void onCreate() {
		super.onCreate();
		logger.info("Service starting");

		HandlerThread handlerThread = new HandlerThread("MyNewThread");
		handlerThread.start();
		// Now get the Looper from the HandlerThread so that we can create a Handler that is attached to
		//  the HandlerThread
		// NOTE: This call will block until the HandlerThread gets control and initializes its Looper
		Looper looper = handlerThread.getLooper();
		// Create a handler for the service
		mHandler = new Handler(looper);
		IntentFilter filter = new IntentFilter();
		filter.addAction(ALARM_ACTION);
		filter.addDataScheme("foo");
		registerReceiver(alarmReceiver, filter, null, mHandler);
		
		alarmManager = (AlarmManager)(this.getSystemService( Context.ALARM_SERVICE ));
		pendingIntent = PendingIntent.getBroadcast( this, 0, new Intent(ALARM_ACTION, Uri.parse("foo:normal")), 0);
		pendingIntentWakeup = PendingIntent.getBroadcast( this, 0, new Intent(ALARM_ACTION, Uri.parse("foo:wake")), 0);
		//alarmManager.set( AlarmManager.ELAPSED_REALTIME_WAKEUP, SystemClock.elapsedRealtime() + INITIAL_DELAY, pendingIntent );

		mSipStack = new SipStack();
		configure();
		mSipStack.init("sip:" + mSipAddr, mSipRealm, mSipUser, mSipPassword);
		mSipStack.setMessageHandler(mh);
		mSipStack.setSipCallFactory(new MySipCallFactory(mCallbacks, this));
		setAlarms(INITIAL_DELAY);
	}

	@Override
	public void onDestroy() {
		super.onDestroy();
		logger.info("Service finishing");

		cancelAlarms();
		pendingIntent = null;
		unregisterReceiver(alarmReceiver);
		
		mSipStack.done();
	}

	private final BroadcastReceiver alarmReceiver = new BroadcastReceiver() {
		@Override
		public void onReceive(Context context, Intent intent) {
			String action = intent.getAction();
			if (action.equals(ALARM_ACTION)) {
				// do it
				runEventLoop();
			}
		}
	};
	
	private final SipStackRemote.Stub mBinder = new SipStackRemote.Stub() {
		@Override
		public void registerUser(SipUserRemote u) {
			mCallbacks.register(u);
		}
		@Override
		public void sendMessage(String recipient, String body) {
			synchronized(mSipStack) {
				mSipStack.sendMessage("sip:" + recipient, body);
			}
		}
		@Override
		public void call(String recipient) {
			logger.info("trying to call: " + recipient);
			synchronized(mSipStack) {
				mSipStack.call("sip:" + recipient);
			}
		}
		@Override
		public void provideOffer(String offer) {
			logger.info("providing offer: " + offer);
			synchronized(mSipCall) {
				mSipCall.provideOffer(offer);
			}
		}
		@Override
		public void provideAnswer(String answer) {
			logger.info("providing answer: " + answer);
			synchronized(mSipCall) {
				mSipCall.provideAnswer(answer);
			}
		}
	};
	
	private final MessageHandler mh = new MessageHandler() {
		@Override
		public void onMessage(String sender, String body) {
			logger.info("Got message from SIP stack: " + body);

			int callbackCount = mCallbacks.beginBroadcast();
			for(int i = 0; i < callbackCount; i++) {
				try {
					mCallbacks.getBroadcastItem(i).onMessage(sender, body);
				} catch (RemoteException ex) {
					logger.throwing("MessageHandler", "onMessage", ex);
				}
			}
			mCallbacks.finishBroadcast();

			// FIXME: is SMS_RECEIVED_ACTION best?  Check names of Extras.
			Intent messageIntent = new Intent(android.provider.Telephony.Sms.Intents.SMS_RECEIVED_ACTION);
			messageIntent.putExtra("sender", sender);
			messageIntent.putExtra("body", body);
			sendBroadcast(messageIntent);
			
			// Make a notification - one of the less elegant things
			// to do in Android programming

			/* FIXME - move this code from this library to the app

			Uri soundUri = RingtoneManager.getDefaultUri(RingtoneManager.TYPE_NOTIFICATION);
			NotificationManager notificationManager = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
			int icon = R.drawable.ic_launcher;
			long ts = System.currentTimeMillis();
			
			Intent notificationIntent = new Intent(SipService.this, MainActivity.class);
			notificationIntent.putExtra("sender", sender);
			PendingIntent contentIntent = PendingIntent.getActivity(SipService.this, 0,
					notificationIntent, PendingIntent.FLAG_UPDATE_CURRENT);

			Notification.Builder mBuilder = new Notification.Builder(getApplicationContext())
				.setWhen(ts)
	        	.setSmallIcon(icon)
	        	.setContentIntent(contentIntent)
	        	.setContentTitle("SIP From: " + sender)
	        	.setContentText(body)
	        	.setTicker(body)
	        	.setSound(soundUri);
			
			Notification notification = mBuilder.getNotification();

			int SERVER_DATA_RECEIVED = 1;
			notificationManager.notify(SERVER_DATA_RECEIVED, notification);*/

		}
	};

	void onNewSipCall(SipCall sipCall) {
		mSipCall = sipCall;
	}
}
