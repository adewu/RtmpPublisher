package github.adewu.rtmppublisher;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaRecorder;
import android.os.Process;
import android.util.Log;

/**
 * Created by wuxm on 04/09/2017.
 * Email 380510218@qq.com
 */

public class LiveAudioRecorder {

    private final static String LOG_TAG = LiveAudioRecorder.class.getSimpleName();

    final int SAMPLE_RATE = 44100; // The sampling rate
    boolean mIsRecording;

    public LiveAudioRecorder() {

    }

    public void startRecording() {
        if (mIsRecording) return;
        mIsRecording = true;
        new Thread(new Runnable() {
            @Override
            public void run() {
                Process.setThreadPriority(Process.THREAD_PRIORITY_AUDIO);
                // buffer size in bytes
                int bufferSize = AudioRecord.getMinBufferSize(SAMPLE_RATE,
                        AudioFormat.CHANNEL_IN_STEREO,
                        AudioFormat.ENCODING_PCM_16BIT);

                if (bufferSize == AudioRecord.ERROR || bufferSize == AudioRecord.ERROR_BAD_VALUE) {
                    bufferSize = SAMPLE_RATE * 2;
                }

                short[] audioBuffer = new short[bufferSize / 2];

                AudioRecord record = new AudioRecord(MediaRecorder.AudioSource.MIC,
                        SAMPLE_RATE,
                        AudioFormat.CHANNEL_IN_MONO,
                        AudioFormat.ENCODING_PCM_16BIT,
                        bufferSize);

                if (record.getState() != AudioRecord.STATE_INITIALIZED) {
                    Log.e(LOG_TAG, "Audio Record can't initialize!");
                    return;
                }
                record.startRecording();

                Log.v(LOG_TAG, "Start recording");

                long shortsRead = 0;
                while (mIsRecording) {
                    int readSize = record.read(audioBuffer, 0, audioBuffer.length);
                    shortsRead += readSize;
                    if (AudioRecord.ERROR_INVALID_OPERATION != readSize) {
                    }

                }

                record.stop();
                record.release();

                Log.v(LOG_TAG, String.format("Recording stopped. Samples read: %d", shortsRead));
            }
        }).start();

    }

    public void stopRecording() {
        if (mIsRecording)
            mIsRecording = false;
    }

    public void onActivityPause() {
        stopRecording();
    }
}
