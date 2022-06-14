package com.GabelStudio.MiniGames;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;

import android.app.Activity;
import android.app.AlertDialog;
import android.content.Context;
import android.content.Intent;
import android.os.AsyncTask;
import android.os.Bundle;
import android.os.Handler;
import android.os.Message;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;
import android.widget.Toast;

import com.GabelStudio.MiniGames.databinding.ActivityMainBinding;


public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    enum JOIN_ACTIONS
    {
        MOVE_TO_LOBBY,
        LOBBY_ACTION,
    };

    // Used to load the 'MiniGames' library on application startup.
    static {
        System.loadLibrary("MiniGames");
    }

    private ActivityMainBinding binding;
    private Handler mainActivityHandler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        Button btn = (Button)findViewById(R.id.joinBtn);
        btn.setOnClickListener(this);

        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_YES);
        // Initialize();
        new Task1().execute();
    }



    @Override
    public void onClick(View view){
        Log.e("TAG", "onClick: HIER");
        MessageServer(mainActivityHandler);
        //SetLobbyActivity();
    }


    public void SetLobbyActivity()
    {
        Message msg = new Message();
        msg.obj = 10;
        mainActivityHandler.sendMessage(msg);
        // Intent intent = new Intent(MainActivity.this, LobbyActivity.class);
        // startActivity(intent);
    }

    protected void onResume()
    {
        super.onResume();
        mainActivityHandler = new MainHandler(this);
    }

    /**
     * A native method that is implemented by the 'MiniGames' native library,
     * which is packaged with this application.
     */
    public static native void Initialize();
    public static native void MessageServer(Handler handler);
}

class Task1 extends AsyncTask<Void, Void, Void>{

    public Task1()
    {
    }

    @Override
    protected void onPreExecute()
    {
        super.onPreExecute();
    }
    @Override
    protected Void doInBackground(Void... arg0)
    {
        MainActivity.Initialize();
        return null;
    }

    @Override
    protected void onPostExecute(Void arg)
    {
        super.onPostExecute(arg);
    }
}

class MainHandler extends Handler
{
    private Activity handlerActivity;
    public MainHandler(Activity act)
    {
        super();
        handlerActivity = act;
    }

    public void TestFunc(int value)
    {
        Log.e( "Tag", "TestFunc: IN DER TEST FUNCITON " + Integer.toString(value));
    }

    @Override
    public void handleMessage(Message msg){
        Intent intent = new Intent(handlerActivity, LobbyActivity.class);
        int val = (int)msg.obj;
        if(val == 10) {
            Context ctx = handlerActivity.getApplicationContext();
            int duration = Toast.LENGTH_SHORT;
            Toast toast = Toast.makeText(ctx, "text", duration);
            toast.show();

            handlerActivity.startActivity(intent);
        }
    }
}