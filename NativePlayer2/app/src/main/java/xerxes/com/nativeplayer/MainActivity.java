package xerxes.com.nativeplayer;

import android.os.Bundle;
import android.support.v7.app.ActionBarActivity;
import android.view.Menu;
import android.view.MenuItem;

import xerxes.com.jnipkg.BufferJNI;


public class MainActivity extends ActionBarActivity {
    private BufferJNI bufferJNI;
    private final String rtmpUrl = "rtmpe://liive.jagobd.com/live";
    private final String appName = "tlive";
    private final String SWFUrl = "http://tv.jagobd.com/player/player.swf";
    private final String pageUrl = "http://www.mcaster.tv/channel/somoynews.";
    private final String playPath = "mp4:somoy-am.stream";
    public static int startFrom = 0;
    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        bufferJNI = new BufferJNI();
        new Thread(new Runnable() {
            @Override
            public void run() {
                // get the filepath
                bufferJNI.startDownload(rtmpUrl,appName,SWFUrl,pageUrl,playPath);
            }
        }).start();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        bufferJNI.stopRecording();
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        // Handle action bar item clicks here. The action bar will
        // automatically handle clicks on the Home/Up button, so long
        // as you specify a parent activity in AndroidManifest.xml.
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_settings) {
            return true;
        }

        return super.onOptionsItemSelected(item);
    }
}
