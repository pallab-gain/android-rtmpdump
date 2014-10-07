package nativeutils;

public class MyRtmp {
	public static native String getStringFromNative();
	static{
		System.loadLibrary("dump");
	}
}
