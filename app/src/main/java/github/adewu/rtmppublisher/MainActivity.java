package github.adewu.rtmppublisher;

import android.Manifest;
import android.content.pm.PackageManager;
import android.media.MediaRecorder;
import android.os.Bundle;
import android.support.annotation.NonNull;
import android.support.v4.app.ActivityCompat;
import android.support.v4.content.ContextCompat;
import android.support.v7.app.AppCompatActivity;
import android.text.TextUtils;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.Toast;

import java.nio.ByteBuffer;

import github.adewu.rtmppublisher.utils.SharedPreferenceUtils;
import github.adewu.rtmppublisher.widgets.PreviewSurfaceView;
import github.adewu.rtmppublisher.BuildConfig;
import github.adewu.rtmppublisher.R;

public class MainActivity extends AppCompatActivity implements View.OnClickListener, PreviewSurfaceView.PreviewFrameListener {

    private github.adewu.rtmppublisher.widgets.PreviewSurfaceView mPreviewSurfaceView;
    private android.widget.Button mStartBtn;
    private android.widget.EditText mUrlET;
    private final static String OUTPUT_URL = "rtmp://192.168.3.133:1935/rtmplive/room1";
    private LiveAudioRecorder mLiveAudioRecorder;
    private int PERMISSION_REQUEST_CAMERA = 95;
    public static boolean hasCameraPermission = false;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        if (ContextCompat.checkSelfPermission(this,
                Manifest.permission.CAMERA)
                != PackageManager.PERMISSION_GRANTED) {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.CAMERA,Manifest.permission.WRITE_EXTERNAL_STORAGE,Manifest.permission.RECORD_AUDIO},
                    PERMISSION_REQUEST_CAMERA);
        } else {
            hasCameraPermission = true;
        }
        setContentView(R.layout.activity_main);
        this.mUrlET = (EditText) findViewById(R.id.url_et);
        this.mStartBtn = (Button) findViewById(R.id.start_btn);
        this.mStartBtn.setOnClickListener(this);
        this.mPreviewSurfaceView = (PreviewSurfaceView) findViewById(R.id.preview_sv);
        mPreviewSurfaceView.setOnPreviewFrameListener(this);
        mUrlET.setText(SharedPreferenceUtils.getStringData(this, "output_url", ""));
        mLiveAudioRecorder = new LiveAudioRecorder();
    }


    @Override
    protected void onResume() {
        super.onResume();
        mPreviewSurfaceView.onActivityResume();
    }

    @Override
    protected void onPause() {
        super.onPause();
        mPreviewSurfaceView.onActivityPause();

    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_CAMERA) {
            if (grantResults.length > 0
                    && grantResults[0] == PackageManager.PERMISSION_GRANTED) {
                mPreviewSurfaceView.startCamera();
            } else {
                finish();
            }
        }
        super.onRequestPermissionsResult(requestCode, permissions, grantResults);
    }


    @Override
    public void onClick(View v) {
        switch (v.getId()) {
            case R.id.start_btn:
                startPublish();
                break;
            default:
                break;
        }

    }

    @Override
    public void previewFrame(byte[] data) {
//        if (BuildConfig.DEBUG) Log.d("PreviewSurfaceView", "data[0]:" + data[0]);
    }

    private void startPublish() {
        String outputUrl = mUrlET.getText().toString();
        if (TextUtils.isEmpty(outputUrl)) {
            Toast.makeText(this, "url cannot be null", Toast.LENGTH_SHORT).show();
            mPreviewSurfaceView.startPublish(OUTPUT_URL);
            mLiveAudioRecorder.startRecording();
            return;
        }
    }


}
