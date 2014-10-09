package nativeutils;

import android.util.Log;

public class MyRtmp {
	private final static String tag = "NativeLoader";
	private static boolean isRecording = false;

	public native int CallMain(String rtmpUrl, String appName, String SWFUrl,
			String pageUrl, String playPath, String filePath);

	public native void stopNativeRecording();

	public static void callback(int nRead) {
		Log.e("FROM-NATIVE", "Callback called " + nRead);
	}

	public int startRecording(String rtmpUrl, String appName, String SWFUrl,
			String pageUrl, String playPath, String filePath) {
		if (isRecording == true) {
			// stop recording
			stopNativeRecording();
		}
		int status = CallMain(rtmpUrl, appName, SWFUrl, pageUrl, playPath,
				filePath);
		return status;
	}

	public void stopRecording() {
		isRecording = false;
		stopNativeRecording();
	}

	static {
		System.loadLibrary("dump");
	}

}
