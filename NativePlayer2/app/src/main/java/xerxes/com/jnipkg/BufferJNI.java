package xerxes.com.jnipkg;

import android.util.Log;

import java.io.FileNotFoundException;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.OutputStream;

import xerxes.com.filemanagerpkg.FileManagerSingleton;

/**
 * Created by xerxes on 3/6/15.
 * "I have not failed, I have just found 10000 ways that won't work."
 */
public class BufferJNI {
    public final String tag = BufferJNI.class.getSimpleName();
    public native void register();
    public native void displayHelloWorld(String s);
    public native int startRecording(String rtmpUrl, String appName, String SWFUrl,
                               String pageUrl, String playPath);
    
    public native void stopRecording();
    OutputStream out;
    public void startDownload(String rtmpUrl, String appName, String SWFUrl, String pageURL, String playPath){
        try {
            out = new FileOutputStream(FileManagerSingleton.getInstance().getOutputFile() );
            startRecording(rtmpUrl, appName, SWFUrl, pageURL, playPath);
        } catch (FileNotFoundException e) {
            e.printStackTrace();
        }
        
    }
    public void stopDownloading(){
        stopRecording();
        try {
            out.close();
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    public void onStreamCallback(final byte[] rtpmStream, final int sz){
        Log.e(tag, "On Stream callback "+rtpmStream.toString()+" "+rtpmStream.length+ " "+sz);
        try {
            out.write(rtpmStream,0,sz);
        } catch (IOException e) {
            e.printStackTrace();
        }
    }
    static {
        System.loadLibrary("toph");
    }
}
