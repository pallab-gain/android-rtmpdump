package com.example.examplejni_4;

import nativeutils.MyRtmp;
import android.app.Activity;
import android.os.Bundle;
import android.util.Log;
import android.view.Menu;
import android.view.MenuItem;

public class MainActivity extends Activity {
	public final String tag = "FromJAVA";

	private final String rtmpUrl = "rtmpe://mobs.jagobd.com/tlive";
	private final String appName = "tlive";
	private final String SWFUrl = "http://tv.jagobd.com/player/player.swf";
	private final String pageUrl = "http://www.mcaster.tv/channel/somoynews.";
	private final String playPath = "mp4:sm.stream";

	@Override
	protected void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
		MyRtmp.CallMain(rtmpUrl, appName, SWFUrl, pageUrl, playPath);
		Log.e(tag, "main function called");
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
}
