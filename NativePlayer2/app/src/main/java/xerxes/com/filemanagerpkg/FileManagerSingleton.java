package xerxes.com.filemanagerpkg;

import android.content.Context;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Environment;

import java.io.File;


/**
 * Created by xerxes on 2/1/15.
 * "I have not failed, I have just found 10000 ways that won't work."
 */
public class FileManagerSingleton {
    private static FileManagerSingleton ourInstance = null;

    public static FileManagerSingleton getInstance() {
        if(ourInstance==null){
            ourInstance = new FileManagerSingleton();
        }
        return ourInstance;
    }

    private FileManagerSingleton() {
    }
    public synchronized File getStorageLocation(){
        if (!Environment.getExternalStorageState().equalsIgnoreCase(
                Environment.MEDIA_MOUNTED)) {
            return null;
        }
        File mediaStorageDir = new File(
                Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_DOWNLOADS),
                "MuffinRTMP");
        if (!mediaStorageDir.exists()) {
            if (!mediaStorageDir.mkdirs()) {
                return null;
            }
        }
        return mediaStorageDir;
    }
    public synchronized File getOutputFile(){
        final File mediaStorageDir = getStorageLocation();
        if(mediaStorageDir==null)return null;

        File mediaFile =  null;
        final long time = System.currentTimeMillis();
        final String filename = Long.toString(time) + ".mp4";
        mediaFile = new File(mediaStorageDir.getPath() + File.separator+ filename);
        return mediaFile;
    }
    /*helper function for test, hardcoded file name*/
    public synchronized File getOutputFile2(final String filename){
        final File mediaStorageDir = getStorageLocation();
        if(mediaStorageDir==null)return null;

        File mediaFile =  null;
        mediaFile = new File(mediaStorageDir.getPath() + File.separator+ filename);
        return mediaFile;
    }

    private long getFileDuration(Context context, final File file){
        try {
            MediaPlayer mp = MediaPlayer.create(context, Uri.fromFile(file));
            return mp.getDuration();
        }catch (Exception e){
            return -1;
        }
    }
    @Deprecated
    public boolean isDurationSatisfy(final Context context, final File file, final long ttl){
        if( getFileDuration(context,file) <= ttl ){
            try {
                file.delete();
            }catch (Exception e){}
            return false;
        }
        return true;
    }
    public boolean isDurationSatisfy(final File file, final long ttl, final long started, final long stopped){
        if( stopped-started < ttl || started > stopped ){
            try{
                file.delete();
            }catch (Exception e){}
            return false;
        }
        return true;
    }
    public boolean isExist(final String fileName){
        try{
            final File file =  new File(fileName);
            return file.exists();
        }catch (Exception e){
            return false;
        }
    }
}