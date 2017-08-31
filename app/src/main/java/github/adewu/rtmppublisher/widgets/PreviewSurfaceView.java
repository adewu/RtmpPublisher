package github.adewu.rtmppublisher.widgets;

import android.content.Context;
import android.content.res.Configuration;
import android.graphics.ImageFormat;
import android.hardware.Camera;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Message;
import android.util.AttributeSet;
import android.util.Log;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import java.io.IOException;
import java.util.List;
import java.util.concurrent.ExecutorService;
import java.util.concurrent.ThreadPoolExecutor;

import github.adewu.rtmppublisher.BuildConfig;
import github.adewu.rtmppublisher.MainActivity;


/**
 * Created by wuxm on 22/08/2017.
 * Email 380510218@qq.com
 */

public class PreviewSurfaceView extends SurfaceView implements SurfaceHolder.Callback, Camera.PreviewCallback {

    static {
        System.loadLibrary("publisher-lib");
    }

    public native int close();

    public native int flush();

    public native int initFFmpeg(int width, int height, String url);

    public native int encode(byte[] data);

    private Camera mCamera;
    private SurfaceHolder mSurfaceHolder;
    private int mPreviewWidth = 640;
    private int mPreviewHeight = 360;
    private int VFPS = 24;
    private int mPreviewOrientation = Configuration.ORIENTATION_PORTRAIT;
    private int mPreviewRotation = 90;
    private int mCamId = -1;
    private PreviewFrameListener mPreviewFrameListener;
    private boolean mIsPublishing = false;

    public PreviewSurfaceView(Context context) {
        super(context);
        init();
    }

    public PreviewSurfaceView(Context context, AttributeSet attrs) {
        super(context, attrs);
        init();
    }

    public PreviewSurfaceView(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init();
    }

    public PreviewSurfaceView(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init();
    }

    @Override
    public void surfaceCreated(SurfaceHolder holder) {
        if (MainActivity.hasCameraPermission)
            startCamera();
    }

    @Override
    public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {

    }

    @Override
    public void surfaceDestroyed(SurfaceHolder holder) {

    }


    private class EncodeThread extends Thread {

    }

    private HandlerThread mEncodeHandlerTHread = new HandlerThread("encode");

    @Override
    public void onPreviewFrame(byte[] data, Camera camera) {
        if (mIsPublishing) {

            mDataBundle.remove("data");
            mDataBundle.putByteArray("data",data);
            android.os.Message message = mEncodeHandler.obtainMessage();
            message.setData(mDataBundle);
            mEncodeHandler.sendMessage(message);
        } else
            mPreviewFrameListener.previewFrame(data);
    }

    private Handler mEncodeHandler;
    private Message mEncodeMessage;
    private Bundle mDataBundle;

    public void startPublish(String url) {
        if (0 == initFFmpeg(mCamera.getParameters().getPreviewSize().width, mCamera.getParameters().getPreviewSize().height, url)) {
            if (BuildConfig.DEBUG) Log.d("PreviewSurfaceView", "FFmpeg initial successed!");
            mIsPublishing = true;
            mEncodeMessage = new Message();
            mDataBundle = new Bundle();
            mEncodeHandlerTHread.start();
            mEncodeHandler = new Handler(mEncodeHandlerTHread.getLooper()) {
                @Override
                public void handleMessage(Message msg) {
                    encode(msg.getData().getByteArray("data"));

                }
            };
        }
    }

    public void onActivityResume() {

    }

    public void onActivityPause() {
        mIsPublishing = false;
        flush();
        close();
    }

    public void setOnPreviewFrameListener(PreviewFrameListener listener) {
        mPreviewFrameListener = listener;
    }

    public boolean startCamera() {
        if (mCamera == null) {
            mCamera = openCamera();
            if (mCamera == null) {
                return false;
            }
        }

        Camera.Parameters params = mCamera.getParameters();
        int[] resoulution = setPreviewResolution(mPreviewWidth, mPreviewHeight);
        params.setPictureSize(resoulution[0], resoulution[1]);
        params.setPreviewSize(resoulution[0], resoulution[1]);
        int[] range = adaptFpsRange(VFPS, params.getSupportedPreviewFpsRange());
        params.setPreviewFpsRange(range[0], range[1]);
        params.setPreviewFormat(ImageFormat.NV21);
        params.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
        params.setSceneMode(Camera.Parameters.SCENE_MODE_AUTO);

        List<String> supportedFocusModes = params.getSupportedFocusModes();
        if (supportedFocusModes != null && !supportedFocusModes.isEmpty()) {
            if (supportedFocusModes.contains(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE)) {
                params.setFocusMode(Camera.Parameters.FOCUS_MODE_CONTINUOUS_PICTURE);
            } else if (supportedFocusModes.contains(Camera.Parameters.FOCUS_MODE_AUTO)) {
                params.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
                mCamera.autoFocus(null);
            } else {
                params.setFocusMode(supportedFocusModes.get(0));
            }
        }

        mCamera.setParameters(params);

        mCamera.setDisplayOrientation(mPreviewRotation);

        try {
            mCamera.setPreviewDisplay(mSurfaceHolder);
        } catch (IOException e) {
            e.printStackTrace();
        }
        mCamera.setPreviewCallback(this);
        mCamera.startPreview();

        return true;
    }

    private void init() {
        mSurfaceHolder = this.getHolder();
        mSurfaceHolder.addCallback(this);
    }


    private Camera openCamera() {
        Camera camera;
        if (mCamId < 0) {
            Camera.CameraInfo info = new Camera.CameraInfo();
            int numCameras = Camera.getNumberOfCameras();
            int frontCamId = -1;
            int backCamId = -1;
            for (int i = 0; i < numCameras; i++) {
                Camera.getCameraInfo(i, info);
                if (info.facing == Camera.CameraInfo.CAMERA_FACING_BACK) {
                    backCamId = i;
                } else if (info.facing == Camera.CameraInfo.CAMERA_FACING_FRONT) {
                    frontCamId = i;
                    break;
                }
            }
            if (frontCamId != -1) {
                mCamId = frontCamId;
            } else if (backCamId != -1) {
                mCamId = backCamId;
            } else {
                mCamId = 0;
            }
        }
        camera = Camera.open(mCamId);
        return camera;
    }

    private int[] adaptFpsRange(int expectedFps, List<int[]> fpsRanges) {
        expectedFps *= 1000;
        int[] closestRange = fpsRanges.get(0);
        int measure = Math.abs(closestRange[0] - expectedFps) + Math.abs(closestRange[1] - expectedFps);
        for (int[] range : fpsRanges) {
            if (range[0] <= expectedFps && range[1] >= expectedFps) {
                int curMeasure = Math.abs(range[0] - expectedFps) + Math.abs(range[1] - expectedFps);
                if (curMeasure < measure) {
                    closestRange = range;
                    measure = curMeasure;
                }
            }
        }
        return closestRange;
    }

    private int[] setPreviewResolution(int width, int height) {
        getHolder().setFixedSize(width, height);

        mCamera = openCamera();
        mPreviewWidth = width;
        mPreviewHeight = height;
        Camera.Size rs = adaptPreviewResolution(mCamera.new Size(width, height));
        if (rs != null) {
            mPreviewWidth = rs.width;
            mPreviewHeight = rs.height;
        }
        mCamera.getParameters().setPreviewSize(mPreviewWidth, mPreviewHeight);

        return new int[]{mPreviewWidth, mPreviewHeight};
    }

    private Camera.Size adaptPreviewResolution(Camera.Size resolution) {
        float diff = 100f;
        float xdy = (float) resolution.width / (float) resolution.height;
        Camera.Size best = null;
        for (Camera.Size size : mCamera.getParameters().getSupportedPreviewSizes()) {
            if (size.equals(resolution)) {
                return size;
            }
            float tmp = Math.abs(((float) size.width / (float) size.height) - xdy);
            if (tmp < diff) {
                diff = tmp;
                best = size;
            }
        }
        return best;
    }

    public interface PreviewFrameListener {
        void previewFrame(byte[] data);
    }
}

