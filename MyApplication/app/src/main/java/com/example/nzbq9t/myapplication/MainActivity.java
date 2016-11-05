package com.example.nzbq9t.myapplication;

import android.os.StrictMode;
import android.renderscript.ScriptGroup;
import android.support.v7.app.AlertDialog;
import android.support.v7.app.AppCompatActivity;
import android.os.Bundle;
import android.util.Log;
import android.view.View;
import android.widget.Button;
import android.widget.EditText;
import android.widget.TextView;

import java.io.BufferedReader;
import java.io.ByteArrayInputStream;
import java.io.ByteArrayOutputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.InputStreamReader;
import java.io.ObjectInputStream;
import java.io.ObjectOutputStream;
import java.io.OutputStream;
import java.io.OutputStreamWriter;
import java.io.PrintWriter;
import java.io.Serializable;
import java.net.InetAddress;
import java.net.Socket;
import java.net.SocketAddress;
import java.nio.Buffer;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.DoubleBuffer;
import java.util.Timer;
import java.util.TimerTask;


public class MainActivity extends AppCompatActivity
{
    Socket mRecv;
    Socket mSend;

    EditText mOutput;
    EditText mInput1;
    EditText mInput2;

    private static String SERVER_IP = "172.27.14.204";//"10.0.2.2";
    private static int SENDPORT = 9000;
    private static int RECVPORT = 9001;

    Packet mObject;
    Timer mRecvData;

    class RecvTask extends TimerTask
    {
        byte[] mBuffer;
        int mOffset;
        Object mRecvMutex;

        RecvTask()
        {
            mRecvMutex = new Object();
            mBuffer = new byte[1024];
            mOffset = 0;
        }

        public void run()
        {
            Log.d("Recv", "Start Recieving");
            if (mRecv == null)
            {
                try
                {
                    Log.d("Thread", "New Recv Connection");
                    mRecv = new Socket(SERVER_IP, RECVPORT);
                }
                catch (Exception e)
                {
                    Log.d("Thread", "Error!");
                    e.printStackTrace();
                }
            }
            else
            {
                try
                {
                    if (mRecv.getInputStream().available() > 0)
                    {
                        InputStream in = mRecv.getInputStream();
                        synchronized (mRecvMutex)
                        {
                            Log.d("Thread", "Reading data");
                            int BytesRead = in.read(mBuffer, mOffset, mBuffer.length - mOffset);

                            String Resp = new String();
                            for (int i = 0; i < BytesRead; i++)
                            {
                                Resp += mBuffer[i] + "-";
                            }
                            Log.d("Recv", Resp);
                            //mOffset += BytesRead;
                        }

                        //BufferedReader in = new BufferedReader(new InputStreamReader(mRecv.getInputStream()));
                        //Log.d("Socket", "Data: " + in.readLine());

                    }
                }
                catch (Exception e)
                {
                    Log.d("Thread", "Error!");
                    e.printStackTrace();
                }
            }
        }

        byte[] GetBytes()
        {
            synchronized (mRecvMutex)
            {
                byte[] Return = new byte[mOffset];
                System.arraycopy(mBuffer, 0, Return, 0, mOffset);
                mOffset = 0;
                return Return;
            }
        }
    }

    @Override
    protected void onCreate(Bundle savedInstanceState)
    {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);

        if (android.os.Build.VERSION.SDK_INT > 9)
        {
            StrictMode.ThreadPolicy policy = new StrictMode.ThreadPolicy.Builder().permitAll().build();
            StrictMode.setThreadPolicy(policy);
        }

        mObject = new Packet();
        mObject.ID1 = 10;
        mObject.ID2 = 20;
        mObject.RSSi = 30;

        mOutput = (EditText)findViewById(R.id.txtOutput);
        mInput1 = (EditText)findViewById(R.id.txtInput1);
        mInput2 = (EditText)findViewById(R.id.txtInput2);

        mRecvData = new Timer();
        mRecvData.schedule(new RecvTask(), 0, 100);
    }

    public void btnGo(View v)
    {
        if (mSend == null)
        {
            try
            {
                Log.d("Thread", "New Send Connection");
                mSend = new Socket(SERVER_IP, SENDPORT);
            }
            catch (Exception e)
            {
                e.printStackTrace();
                return;
            }
        }

        mObject.ID1 = Integer.parseInt(mInput1.getText().toString());
        mObject.ID2 = Integer.parseInt(mInput2.getText().toString());
        mObject.RSSi = Integer.parseInt(mOutput.getText().toString());

        Send(mObject);
        Log.d("Socket", "Successful Send");
    }

    private void Send(Packet Object)
    {
        ByteBuffer Buffer = ByteBuffer.allocate(12);
        Buffer.order(ByteOrder.LITTLE_ENDIAN);

        Buffer.putInt(Object.ID1);
        Buffer.putInt(Object.ID2);
        Buffer.putInt(Object.RSSi);

        try
        {
            byte[] SendBuffer = Buffer.array();
            OutputStream out = mSend.getOutputStream();
            DataOutputStream dos = new DataOutputStream(out);
            dos.write(SendBuffer, 0, SendBuffer.length);
        }
        catch (Exception e)
        {
            e.printStackTrace();
            return;
        }
    }

    public void btnCalculation(View v)
    {

    }
}