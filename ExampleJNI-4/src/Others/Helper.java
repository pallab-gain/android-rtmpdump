package Others;

import java.io.File;

import android.os.Environment;
import android.util.Log;

public class Helper {
	public static String filePath = null;

	public static File getOutputMediaFile() {
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
				Log.d("BD-TV", "failed to create directory");
				return null;
			}
		}

		String videoFileName = "tv-temp.mp4";
		// name will be deviceID and date millisecond like
		File mediaFile = new File(mediaStorageDir.getPath() + File.separator
				+ videoFileName);
		if (mediaFile.exists()) {
			mediaFile.delete();
		}
		filePath = mediaFile.toString();
		return mediaFile;
	}

	public static void removeFile() {
		if (filePath != null) {
			File mediaFile = new File(getFilePath());
			mediaFile.delete();
		}

	}

	public static String getFilePath() {
		return filePath;
	}
}
