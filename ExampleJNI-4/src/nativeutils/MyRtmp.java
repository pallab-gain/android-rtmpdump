package nativeutils;

import android.util.Log;

public class MyRtmp {
	private final static String tag = "NativeLoader";
	public native int CallMain(String rtmpUrl, String appName, String SWFUrl,
			String pageUrl, String playPath);
	
	public static void callback(){
		Log.e("FROM-NATIVE","Callback called");
	}
	static {
		System.loadLibrary("dump");
	}

}
