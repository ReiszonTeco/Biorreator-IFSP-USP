package com.example.bt_esp32;

import androidx.appcompat.app.AppCompatActivity;
import android.graphics.Color;
import android.os.Bundle;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;
import android.widget.TextView;
import com.github.mikephil.charting.charts.LineChart;
import com.github.mikephil.charting.components.Description;
import com.github.mikephil.charting.data.Entry;
import com.github.mikephil.charting.data.LineData;
import com.github.mikephil.charting.data.LineDataSet;
import java.util.ArrayList;

public class MonitorActivity extends AppCompatActivity {

    private static final String TAG = "MonitorActivity";
    private TextView tvStatusCiclo, tvTemperatura;
    private TextView tvStatusAgitador, tvStatusAerador, tvStatusBomba1, tvStatusBomba2;
    private LineChart lineChart;
    private ArrayList<Entry> chartData;
    private long startTime = 0;

    public static Handler handler;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_monitor);

        vincularComponentesUI();
        configurarGrafico();
        startTime = System.currentTimeMillis();

        // Handler para receber os pacotes de dados da MainActivity
        handler = new Handler(Looper.getMainLooper(), msg -> {
            if (msg.what == 2) { // Código de mensagem para o monitor
                String packet = (String) msg.obj;
                runOnUiThread(() -> processarPacoteDeDados(packet));
            }
            return true;
        });
    }

    private void vincularComponentesUI() {
        tvStatusCiclo = findViewById(R.id.tvStatusCiclo);
        tvTemperatura = findViewById(R.id.tvTemperatura);
        tvStatusAgitador = findViewById(R.id.tvStatusAgitador);
        tvStatusAerador = findViewById(R.id.tvStatusAerador);
        tvStatusBomba1 = findViewById(R.id.tvStatusBomba1);
        tvStatusBomba2 = findViewById(R.id.tvStatusBomba2);
        lineChart = findViewById(R.id.lineChart);
    }

    private void configurarGrafico() {
        chartData = new ArrayList<>();
        Description description = new Description();
        description.setText("Temperatura (°C) vs Tempo (s)");
        lineChart.setDescription(description);
        lineChart.setNoDataText("Aguardando dados...");
    }

    private void processarPacoteDeDados(String pacote) {
        Log.d(TAG, "Processando pacote: " + pacote);
        if (pacote.startsWith("D;") && pacote.endsWith(";T")) {
            String dadosLimpos = pacote.substring(2, pacote.length() - 2);
            String[] partes = dadosLimpos.split(";");

            if (partes.length == 6) {
                try {
                    float temp = Float.parseFloat(partes[0]);
                    tvTemperatura.setText(String.format("%.1f °C", temp));
                    atualizarGrafico(temp);

                    atualizarStatusAtuador(tvStatusAgitador, "Agitador", partes[1]);
                    atualizarStatusAtuador(tvStatusAerador, "Aeração", partes[2]);
                    atualizarStatusAtuador(tvStatusBomba1, "Bomba 1", partes[3]);
                    atualizarStatusAtuador(tvStatusBomba2, "Bomba 2", partes[4]);

                    tvStatusCiclo.setText(partes[5]);

                } catch (NumberFormatException e) {
                    Log.e(TAG, "Erro ao converter dados do pacote: " + pacote);
                }
            }
        }
    }

    private void atualizarStatusAtuador(TextView textView, String nome, String estado) {
        if (estado.equals("1")) {
            textView.setText(nome + ": LIGADO");
            textView.setTextColor(Color.parseColor("#4CAF50"));
        } else {
            textView.setText(nome + ": DESLIGADO");
            textView.setTextColor(Color.parseColor("#F44336"));
        }
    }

    private void atualizarGrafico(float temp) {
        long tempoDecorrido = (System.currentTimeMillis() - startTime) / 1000;
        chartData.add(new Entry(tempoDecorrido, temp));

        LineDataSet dataSet = new LineDataSet(chartData, "Temperatura");
        dataSet.setColor(Color.BLUE);
        dataSet.setCircleColor(Color.BLUE);
        dataSet.setDrawValues(false);

        LineData lineData = new LineData(dataSet);
        lineChart.setData(lineData);
        lineChart.invalidate();
    }
}
