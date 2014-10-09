package com.example.examplejni_4;

import nativeutils.MyRtmp;
import android.app.Activity;
import android.app.ProgressDialog;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.MediaController;
import android.widget.ToggleButton;
import android.widget.VideoView;

public class MainActivity extends Activity {
	public final String tag = "FromJAVA";
	MyRtmp rtmp = new MyRtmp();

	private final String rtmpUrl = "rtmpe://mobs.jagobd.com/tlive";
	private final String appName = "tlive";
	private final String SWFUrl = "http://tv.jagobd.com/player/player.swf";
	private final String pageUrl = "http://www.mcaster.tv/channel/somoynews.";
	private final String playPath = "mp4:sm.stream";
	public static String filePath = null;
	ToggleButton toggleButton;

	// START ( VIDEO VIEW VARIABLES )
	private VideoView mVideoView = null;

	// END ( VIDEO VIEW VARIABLES )
	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		toggleButton = (ToggleButton) findViewById(R.id.toggleButton1);
		mVideoView = (VideoView) findViewById(R.id.video_view);

		toggleButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				if (((ToggleButton) view).isChecked()) {
					Log.e(tag, "start recording");
					// start recoding

					new Thread(new Runnable() {

						@Override
						public void run() {
							filePath = Others.Helper.getOutputMediaFile()
									.toString();
							int status = rtmp.startRecording(rtmpUrl, appName,
									SWFUrl, pageUrl, playPath, filePath);
							Log.e(tag, "" + status);
						}
					}).start();
					while (true) {
						if (filePath != null) {
							runOnUiThread(new Runnable() {
								@Override
								public void run() {
									// TODO Auto-generated method stub
									playVideo();
								}
							});
							break;
						}
					}

				} else {
					Log.e(tag, "stop recording");
					// stop recording
					rtmp.stopRecording();
					if (mVideoView != null) {
						mVideoView.stopPlayback();
					}

				}
			}
		});
	}

	@Override
	public boolean onCreateOptionsMenu(Menu menu) {
		// Inflate the menu; this adds items to the action bar if it is present.
		getMenuInflater().inflate(R.menu.main, menu);
		return true;
	}

	@Override
	public boolean onOptionsItemSelected(MenuItem item) {
		// Handle action bar item clicks here. The action bar will
		// automatically handle clicks on the Home/Up button, so long
		// as you specify a parent activity in AndroidManifest.xml.
		int id = item.getItemId();
		if (id == R.id.action_settings) {
			return true;
		}
		return super.onOptionsItemSelected(item);
	}

	private void playVideo() {
		try {
			// If the path has not changed, just start the media player
			mVideoView.setVideoPath(getDataSource());
			mVideoView.start();
			mVideoView.requestFocus();
		} catch (Exception e) {
			Log.e(tag, "Exception " + e.toString());
			if (mVideoView != null) {
				mVideoView.stopPlayback();
			}
		}
	}

	private String getDataSource() {
		return filePath;
	}
}
