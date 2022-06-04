package com.GabelStudio.MiniGames;

import androidx.appcompat.app.AppCompatActivity;
import androidx.appcompat.app.AppCompatDelegate;

import android.app.Activity;
import android.content.Intent;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.view.ViewGroup;
import android.widget.Button;
import android.widget.TextView;

import com.GabelStudio.MiniGames.databinding.ActivityMainBinding;

import java.io.IOException;
import java.net.Socket;

public class MainActivity extends AppCompatActivity implements View.OnClickListener {

    // Used to load the 'MiniGames' library on application startup.
    static {
        System.loadLibrary("MiniGames");
    }

    private ActivityMainBinding binding;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        binding = ActivityMainBinding.inflate(getLayoutInflater());
        setContentView(binding.getRoot());

        Button btn = (Button)findViewById(R.id.joinBtn);
        btn.setOnClickListener(this);

        AppCompatDelegate.setDefaultNightMode(AppCompatDelegate.MODE_NIGHT_NO);

        stringFromJNI();
    }



    @Override
    public void onClick(View view){
        Log.e("TAG", "onClick: ERROR");
        Intent intent = new Intent(MainActivity.this, LobbyActivity.class);
        startActivity(intent);

    }

    /**
     * A native method that is implemented by the 'MiniGames' native library,
     * which is packaged with this application.
     */
    public native String stringFromJNI();
}