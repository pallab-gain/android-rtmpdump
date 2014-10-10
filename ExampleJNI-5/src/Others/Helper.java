package Others;

import java.io.File;

import android.os.Environment;
import android.util.Log;

public class Helper {
	public static String getFilePath() {
        if (!Environment.getExternalStorageState().equalsIgnoreCase(
                Environment.MEDIA_MOUNTED)) {
            return null;
        }
        File mediaStorageDir = new File(
                Environment
                        .getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES),
                "BD-TV");
        
        if (!mediaStorageDir.exists()) {
            if (!mediaStorageDir.mkdirs()) {
                Log.e("HelperCLass", "failed to create directory");
                return null;
            }
        }
        String fileName = "tv-temp.mp4";
        return mediaStorageDir.toString() + "/" + fileName;
    }

	public static void removeFile() {
		final String filePath = getFilePath();
		if (filePath != null) {
			File mediaFile = new File(filePath);
			mediaFile.delete();
		}
	}
}
