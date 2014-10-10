package com.example.examplejni_4;

import java.io.File;
import java.io.IOException;

import nativeutils.MyRtmp;
import Others.Helper;
import android.annotation.SuppressLint;
import android.app.Activity;
import android.content.Context;
import android.graphics.SurfaceTexture;
import android.media.MediaPlayer;
import android.net.Uri;
import android.os.Bundle;
import android.os.Environment;
import android.os.Handler;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;
import android.view.Surface;
import android.view.TextureView;
import android.view.View;
import android.widget.SeekBar;
import android.widget.ToggleButton;
import android.widget.VideoView;

@SuppressLint("NewApi")
public class MainActivity extends Activity {
	private static final String tag = "MainActivity";
	private static MediaPlayer mMediaPlayer = null;
	private static TextureView mVideoView = null;
	private static Context ctx = null;
	private static Surface surface = null;
	ToggleButton mToggleButton = null;
	MyRtmp mRtmp;

	private final String rtmpUrl = "rtmpe://mobs.jagobd.com/tlive";
	private final String appName = "tlive";
	private final String SWFUrl = "http://tv.jagobd.com/player/player.swf";
	private final String pageUrl = "http://www.mcaster.tv/channel/somoynews.";
	private final String playPath = "mp4:sm.stream";
	public static int startFrom = 0;

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		if (mVideoView == null)
			mVideoView = (TextureView) findViewById(R.id.textureView);
		if (mToggleButton == null)
			mToggleButton = (ToggleButton) findViewById(R.id.toggleButton);
		if (ctx == null)
			ctx = getApplicationContext();
		if (mRtmp == null) {
			mRtmp = new MyRtmp();
		}

		mVideoView.setSurfaceTextureListener(mSurfaceTextureListener);
		mToggleButton.setOnClickListener(mClickButtonListner);
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

	// listner
	private TextureView.SurfaceTextureListener mSurfaceTextureListener = new TextureView.SurfaceTextureListener() {
		@Override
		public void onSurfaceTextureAvailable(SurfaceTexture surface,
				int width, int height) {
			Log.e(tag, "Surface texture available");
			// mSurfaceTextureSubject.onNext(surface);
		}

		@Override
		public void onSurfaceTextureSizeChanged(SurfaceTexture surface,
				int width, int height) {
			Log.e(tag, "Surface texture size change");
			// correctVideoAspectRatio();
		}

		@Override
		public boolean onSurfaceTextureDestroyed(SurfaceTexture surface) {
			Log.e(tag, "Surface texture destroyed");
			// mSurfaceTextureSubject.onNext(null);
			stopPlayer();
			if (mRtmp != null) {
				mRtmp.stopRecording();
			}
			return false;
		}

		@Override
		public void onSurfaceTextureUpdated(SurfaceTexture surface) {
			// Log.e(tag, "Surface texture updated");
		}
	};

	private static MediaPlayer.OnPreparedListener mPrepareListner = new MediaPlayer.OnPreparedListener() {
		@Override
		public void onPrepared(MediaPlayer mediaPlayer) {
			mediaPlayer.seekTo(startFrom);
			mediaPlayer.start();
			Log.e(tag,
					"Player is ready to play,duration got "
							+ mediaPlayer.getDuration());
		}
	};
	private static MediaPlayer.OnCompletionListener mCompleteListner = new MediaPlayer.OnCompletionListener() {
		@Override
		public void onCompletion(MediaPlayer mediaPlayer) {
			// TODO Auto-generated method stub
			Log.e(tag, "Completed bang " + mediaPlayer.getDuration());
			startFrom = mediaPlayer.getDuration();
			startPlayer(Helper.getFilePath());
		}
	};
	private View.OnClickListener mClickButtonListner = new View.OnClickListener() {
		@Override
		public void onClick(View view) {
			if (((ToggleButton) view).isChecked() == true) {
				Log.e(tag, "Start Playing");
				startFrom = 0;
				// fresh start media player
				// startPlayer(getOutputMediaFile("lemonade.mp4"));
				final String filePath = Helper.getFilePath();
				if (filePath == null) {
					Log.e(tag, "Cannot create a file");
				} else {
					new Thread(new Runnable() {
						@Override
						public void run() {
							// get the filepath
							if (mRtmp == null) {
								mRtmp = new MyRtmp();
							}
							int status = mRtmp.startRecording(rtmpUrl, appName,
									SWFUrl, pageUrl, playPath, filePath);
							Log.e(tag, "yeo3x " + status);
						}
					}).start();

					new Handler().postDelayed(new Runnable() {
						@Override
						public void run() {
							String fpath = Helper.getFilePath();
							if (fileSize(fpath) > 20000) {
								Log.e(tag, "Play the video now "
										+ fileSize(fpath));
								startPlayer(fpath);
							} else {
								Log.e(tag, "File is too small to play"
										+ fileSize(fpath));
							}
						}
					}, (long) 1000 * 5);
				}

			} else {
				stopPlayer();
				if (mRtmp != null) {
					mRtmp.stopRecording();
				}
				Log.e(tag, "Stop playing");
			}
		}
	};

	// media player helper methods
	public static void startPlayer(String playPath) {
		if (mMediaPlayer != null) {
			mMediaPlayer.release();
			mMediaPlayer = null;
		}
		if (surface != null) {
			surface.release();
			surface = null;
		}
		surface = new Surface(mVideoView.getSurfaceTexture());
		mMediaPlayer = new MediaPlayer();
		mMediaPlayer.setOnPreparedListener(mPrepareListner);
		mMediaPlayer.setOnCompletionListener(mCompleteListner);
		if (playPath != null) {
			Uri url = Uri.parse(playPath);
			Log.e(tag, url.toString());
			try {
				mMediaPlayer.setDataSource(ctx, url);
			} catch (IOException e) {
				Log.e(tag, "Cannot set data source");
				e.printStackTrace();
				return;
			}
			mMediaPlayer.setSurface(surface);
			mMediaPlayer.prepareAsync();
		} else {
			Log.e(tag, "No file to play");
		}
	}

	public static void SpecialResume() {
		Log.e(tag, "bang bang!!!");
		startFrom = 0;
		new Handler().postDelayed(new Runnable() {
			@Override
			public void run() {
				String fpath = Helper.getFilePath();
				if (mMediaPlayer != null) {
					mMediaPlayer.release();
					mMediaPlayer = null;
				}
				if (surface != null) {
					surface.release();
					surface = null;
				}
				surface = new Surface(mVideoView.getSurfaceTexture());
				mMediaPlayer = new MediaPlayer();
				mMediaPlayer.setOnPreparedListener(mPrepareListner);
				mMediaPlayer.setOnCompletionListener(mCompleteListner);
				if (fpath != null) {
					Uri url = Uri.parse(fpath);
					Log.e(tag, url.toString());
					try {
						mMediaPlayer.setDataSource(ctx, url);
					} catch (IOException e) {
						Log.e(tag, "Cannot set data source");
						e.printStackTrace();
						return;
					}
					mMediaPlayer.setSurface(surface);
					mMediaPlayer.prepareAsync();
				} else {
					Log.e(tag, "No file to play");
				}
			}
		}, (long) 1000 * 5);

	}

	private void stopPlayer() {
		if (mMediaPlayer != null) {
			mMediaPlayer.release();
			mMediaPlayer = null;
		}
	}

	private long fileSize(String curFilePath) {
		File file = new File(curFilePath);
		return file.length();
	}

}
