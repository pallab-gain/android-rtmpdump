package com.example.examplejni_4;

import java.io.File;

import nativeutils.MyRtmp;
import Others.Helper;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.app.ProgressDialog;
import android.media.MediaPlayer;
import android.os.Bundle;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.MediaController;
import android.widget.ToggleButton;
import android.widget.VideoView;

@SuppressLint("NewApi")
public class MainActivity extends Activity {
	public final String tag = "FromJAVA";
	MyRtmp rtmp = new MyRtmp();
	VideoView mVideoView;
	static Integer pos = 0;

	private final String rtmpUrl = "rtmpe://mobs.jagobd.com/tlive";
	private final String appName = "tlive";
	private final String SWFUrl = "http://tv.jagobd.com/player/player.swf";
	private final String pageUrl = "http://www.mcaster.tv/channel/somoynews.";
	private final String playPath = "mp4:sm.stream";
	public static String filePath = null;
	ToggleButton toggleButton;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		mVideoView = (VideoView) findViewById(R.id.video_view);
		mVideoView
				.setOnCompletionListener(new MediaPlayer.OnCompletionListener() {

					@Override
					public void onCompletion(MediaPlayer arg0) {
						// TODO Auto-generated method stub
						// try to resume the view
						resumeVideo(mVideoView.getCurrentPosition());
					}

				});
		toggleButton = (ToggleButton) findViewById(R.id.toggleButton1);
		toggleButton.setOnClickListener(new View.OnClickListener() {
			@Override
			public void onClick(View view) {
				if (((ToggleButton) view).isChecked()) {
					Log.e(tag, "start recording");
					// start recoding

					new Thread(new Runnable() {

						@Override
						public void run() {
							// get the filepath
							filePath = Others.Helper.getOutputMediaFile()
									.toString();
							int status = rtmp.startRecording(rtmpUrl, appName,
									SWFUrl, pageUrl, playPath, filePath);
							Log.e(tag, "" + status);
						}
					}).start();

					runOnUiThread(new Runnable() {
						@Override
						public void run() {
							// TODO Auto-generated method stub
							playVideo();
						}
					});

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

	private void resumeVideo(final Integer poos) {
		if (MyRtmp.isRecording == true) {
			String filepath = Helper.getFilePath();
			if (filepath != null) {
				mVideoView.setVideoPath(filepath);
				mVideoView.seekTo(poos);
				mVideoView.start();
			}else{
				;
			}
		} else {
			Log.e(tag, "not recording...!");
		}
	}

	private void playVideo() {
		try {
			final Handler handler = new Handler();
			handler.postDelayed(new Runnable() {
				@Override
				public void run() {
					// Do something after 5s = 5000ms
					if (readyToPlay() == true) {
						String filepath = Helper.getFilePath();
						if (filepath != null) {
							mVideoView.setVideoPath(filepath);
							mVideoView.start();
							mVideoView.requestFocus();
						}
					} else {
						playVideo();
					}
				}
			}, 1000 * 5); // 5 second delay
		} catch (Exception e) {
			Log.e(tag, "Exception " + e.toString());
			if (mVideoView != null) {
				mVideoView.stopPlayback();
			}
		}
	}

	private boolean readyToPlay() {
		if (Helper.getFilePath() == null)
			return false;
		File file = new File(Helper.getFilePath());
		return file.length() >= 1024 ? true : false;
	}
}
