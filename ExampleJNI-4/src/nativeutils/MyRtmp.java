package nativeutils;

public class MyRtmp {
	private final static String tag = "NativeLoader";
	public static native int CallMain(String rtmpUrl, String appName, String SWFUrl,
			String pageUrl, String playPath);
	static {
		System.loadLibrary("dump");
	}

}
