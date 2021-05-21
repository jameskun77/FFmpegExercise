package com.codefun;

import androidx.annotation.NonNull;
import androidx.appcompat.app.AppCompatActivity;
import androidx.core.app.ActivityCompat;
import androidx.recyclerview.widget.LinearLayoutManager;
import androidx.recyclerview.widget.RecyclerView;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.opengl.GLSurfaceView;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.LayoutInflater;
import android.view.Menu;
import android.view.MenuItem;
import android.view.View;
import android.widget.Button;
import android.widget.SeekBar;
import android.widget.Toast;

import com.codefun.adapter.MyRecyclerViewAdapter;
import com.codefun.media.FFMediaPlayer;
import com.codefun.media.MyGLSurfaceView;
import com.codefun.util.CommonUtils;

import java.util.Arrays;

import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

import static com.codefun.media.FFMediaPlayer.MEDIA_PARAM_VIDEO_DURATION;
import static com.codefun.media.FFMediaPlayer.MEDIA_PARAM_VIDEO_HEIGHT;
import static com.codefun.media.FFMediaPlayer.MEDIA_PARAM_VIDEO_WIDTH;
import static com.codefun.media.FFMediaPlayer.MSG_DECODER_DONE;
import static com.codefun.media.FFMediaPlayer.MSG_DECODER_INIT_ERROR;
import static com.codefun.media.FFMediaPlayer.MSG_DECODER_READY;
import static com.codefun.media.FFMediaPlayer.MSG_DECODING_TIME;
import static com.codefun.media.FFMediaPlayer.MSG_REQUEST_RENDER;
import static com.codefun.media.FFMediaPlayer.VIDEO_GL_RENDER;

public class MainActivity extends AppCompatActivity implements GLSurfaceView.Renderer,
                    FFMediaPlayer.EventCallback,MyGLSurfaceView.OnGestureCallback{

    private static final String TAG = "MainActivity";
    private static final String[] REQUEST_PERMISSIONS = {
            Manifest.permission.WRITE_EXTERNAL_STORAGE
    };

    private static final int PERMISSION_REQUEST_CODE = 1;

    private int mSampleSelectedIndex = -1;

    private static final String [] EXAMPLE_LIST = {
            "FFmpeg + OpenGL ES Play",
            "FFmpeg + stream media play"
    };

    private static final int FF_OPENGLES_EXAMPLE = 0;
    private static final int FF_OPENGLES_VR_EXAMPLE = 1;

    private static final int FF_OPENGL_EXAMPLE = 1;

    private FFMediaPlayer mMediaPlayer = null;
    private SeekBar       mSeekBar = null;
    private MyGLSurfaceView mGLSurfaceView = null;
    private boolean mIsTouch = false;
    private String mVideoPath = Environment.getExternalStorageDirectory().getAbsolutePath() +
            "/byteflow/one_piece.mp4";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        ///mGLSurfaceView = new MyGLSurfaceView(this);
        mGLSurfaceView = findViewById(R.id.surface_view);
        mGLSurfaceView.setEGLContextClientVersion(3);
        mGLSurfaceView.setRenderer(this);
        mGLSurfaceView.addOnGestureCallback(this);
        mGLSurfaceView.setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);

        mSeekBar = findViewById(R.id.seek_bar);
        mSeekBar.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
            @Override
            public void onProgressChanged(SeekBar seekBar, int i, boolean b) {

            }

            @Override
            public void onStartTrackingTouch(SeekBar seekBar) {
                mIsTouch = true;
            }

            @Override
            public void onStopTrackingTouch(SeekBar seekBar) {
                Log.d(TAG, "onStopTrackingTouch() called with: progress = [" + seekBar.getProgress() + "]");
                if(mMediaPlayer != null) {
                    mMediaPlayer.seekToPosition(mSeekBar.getProgress());
                    mIsTouch = false;
                }
            }
        });

        mMediaPlayer = new FFMediaPlayer();
        mMediaPlayer.addEventCallback(this);
        mMediaPlayer.init(mVideoPath,FFMediaPlayer.VIDEO_RENDER_OPENGL,null);
    }

    @Override
    protected void onResume() {
        super.onResume();
        mSampleSelectedIndex = -1;
        CommonUtils.copyAssetsDirToSDCard(this, "byteflow", "/sdcard");
        if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
            ActivityCompat.requestPermissions(this, REQUEST_PERMISSIONS, PERMISSION_REQUEST_CODE);
        }

        if(mMediaPlayer != null){
            mMediaPlayer.play();
        }
    }

    @Override
    protected void onPause() {
        super.onPause();
        if(mMediaPlayer != null)
            mMediaPlayer.pause();
    }

    @Override
    protected void onDestroy() {
        super.onDestroy();
        if(mMediaPlayer != null)
            mMediaPlayer.unInit();
    }

    @Override
    public void onRequestPermissionsResult(int requestCode, @NonNull String[] permissions, @NonNull int[] grantResults) {
        if (requestCode == PERMISSION_REQUEST_CODE) {
            if (!hasPermissionsGranted(REQUEST_PERMISSIONS)) {
                Toast.makeText(this, "We need the permission: WRITE_EXTERNAL_STORAGE", Toast.LENGTH_SHORT).show();
            }
        } else {
            super.onRequestPermissionsResult(requestCode, permissions, grantResults);
        }
    }

    @Override
    public void onSurfaceCreated(GL10 gl10, EGLConfig eglConfig) {
        FFMediaPlayer.native_OnSurfaceCreated(VIDEO_GL_RENDER);
    }

    @Override
    public void onSurfaceChanged(GL10 gl10, int w, int h) {
        Log.d(TAG, "onSurfaceChanged() called with: gl10 = [" + gl10 + "], w = [" + w + "], h = [" + h + "]");
        FFMediaPlayer.native_OnSurfaceChanged(VIDEO_GL_RENDER, w, h);
    }

    @Override
    public void onDrawFrame(GL10 gl10) {
        FFMediaPlayer.native_OnDrawFrame(VIDEO_GL_RENDER);
    }

    @Override
    public void onPlayerEvent(final int msgType, final float msgValue) {
        runOnUiThread(new Runnable() {
            @Override
            public void run() {
                switch (msgType) {
                    case MSG_DECODER_INIT_ERROR:
                        break;
                    case MSG_DECODER_READY:
                        onDecoderReady();
                        break;
                    case MSG_DECODER_DONE:
                        break;
                    case MSG_REQUEST_RENDER:
                        mGLSurfaceView.requestRender();
                        break;
                    case MSG_DECODING_TIME:
                        if(!mIsTouch)
                            mSeekBar.setProgress((int) msgValue);
                        break;
                    default:
                        break;
                }
            }
        });
    }

    private void onDecoderReady(){
        int videoWidth = (int) mMediaPlayer.getMediaParams(MEDIA_PARAM_VIDEO_WIDTH);
        int videoHeight = (int) mMediaPlayer.getMediaParams(MEDIA_PARAM_VIDEO_HEIGHT);
        if(videoHeight * videoWidth != 0)
            mGLSurfaceView.setAspectRatio(videoWidth,videoHeight);

        int duration = (int) mMediaPlayer.getMediaParams(MEDIA_PARAM_VIDEO_DURATION);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.O) {
            mSeekBar.setMin(0);
        }
        mSeekBar.setMax(duration);
    }

    @Override
    public void onGesture(int xRotateAngle, int yRotateAngle, float scale) {
        FFMediaPlayer.native_SetGesture(VIDEO_GL_RENDER, xRotateAngle, yRotateAngle, scale);
    }

    @Override
    public void onTouchLoc(float touchX, float touchY) {
        FFMediaPlayer.native_SetTouchLoc(VIDEO_GL_RENDER, touchX, touchY);
    }

    @Override
    public boolean onCreateOptionsMenu(Menu menu) {
        // Inflate the menu; this adds items to the action bar if it is present.
        getMenuInflater().inflate(R.menu.menu_main, menu);
        return true;
    }

    @Override
    public boolean onOptionsItemSelected(MenuItem item) {
        int id = item.getItemId();

        //noinspection SimplifiableIfStatement
        if (id == R.id.action_change_sample) {
            showSelectExampleDialog();
        }
        return true;
    }

    private void showSelectExampleDialog() {
        final AlertDialog.Builder builder = new AlertDialog.Builder(this);
        LayoutInflater inflater = LayoutInflater.from(this);
        final View rootView = inflater.inflate(R.layout.sample_selected_layout, null);

        final AlertDialog dialog = builder.create();

        Button confirmBtn = rootView.findViewById(R.id.confirm_btn);
        confirmBtn.setOnClickListener(new View.OnClickListener() {
            @Override
            public void onClick(View v) {
                dialog.cancel();
            }
        });

        final RecyclerView resolutionsListView = rootView.findViewById(R.id.resolution_list_view);

        final MyRecyclerViewAdapter myPreviewSizeViewAdapter = new MyRecyclerViewAdapter(this, Arrays.asList(EXAMPLE_LIST));
        myPreviewSizeViewAdapter.setSelectIndex(mSampleSelectedIndex);
        myPreviewSizeViewAdapter.addOnItemClickListener(new MyRecyclerViewAdapter.OnItemClickListener() {
            @Override
            public void onItemClick(View view, int position) {
                int selectIndex = myPreviewSizeViewAdapter.getSelectIndex();
                myPreviewSizeViewAdapter.setSelectIndex(position);
                myPreviewSizeViewAdapter.safeNotifyItemChanged(selectIndex);
                myPreviewSizeViewAdapter.safeNotifyItemChanged(position);
                mSampleSelectedIndex = position;
                switch (position) {
                    case FF_OPENGLES_EXAMPLE:
                        //startActivity(new Intent(MainActivity.this, GLMediaPlayerActivity.class));
                        break;
                    case FF_OPENGLES_VR_EXAMPLE:
                        //startActivity(new Intent(MainActivity.this, VRMediaPlayerActivity.class));
                        break;
                    default:
                        break;
                }

                dialog.cancel();
            }
        });

        LinearLayoutManager manager = new LinearLayoutManager(this);
        manager.setOrientation(LinearLayoutManager.VERTICAL);
        resolutionsListView.setLayoutManager(manager);

        resolutionsListView.setAdapter(myPreviewSizeViewAdapter);
        resolutionsListView.scrollToPosition(mSampleSelectedIndex);

        dialog.show();
        dialog.getWindow().setContentView(rootView);

    }

    protected boolean hasPermissionsGranted(String[] permissions) {
        for (String permission : permissions) {
            if (ActivityCompat.checkSelfPermission(this, permission)
                    != PackageManager.PERMISSION_GRANTED) {
                return false;
            }
        }
        return true;
    }

}