let host = null;

window.onload = async () => {
  host = window.location.hostname;

  try {
    const response = await fetch(`http://${host}/api/config`);

    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();

    document.getElementById('interval').value = data.interval;
    document.getElementById('radio-on').checked = data.ledOn;
    document.getElementById('radio-off').checked = !data.ledOn;

    const colors = data.statusColors;

    document.getElementById('color-1').value = colors.standby;
    document.getElementById('color-2').value = colors.sleep;
    document.getElementById('color-3').value = colors.highTemperature;
    document.getElementById('color-4').value = colors.measurementInProcess;
    document.getElementById('color-5').value = colors.noWlan;
  } catch (error) {
    console.error('Error: ' + error);
  }
};

async function saveConfig(event) {
  event.preventDefault();

  const ledPower = document.getElementById('radio-on').checked;
  const intervalMillis = document.getElementById('interval').value;
  const standbyColor = document.getElementById('color-1').value;
  const sleepColor = document.getElementById('color-2').value;
  const highTempColor = document.getElementById('color-3').value;
  const measuringColor = document.getElementById('color-4').value;
  const noWifiColor = document.getElementById('color-5').value;

  try {
    const response = await fetch(`http://${host}/api/config`, {
      method: 'post',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        ledOn: ledPower,
        interval: intervalMillis,
        statusColors: {
          standby: standbyColor,
          sleep: sleepColor,
          highTemperature: highTempColor,
          measurementInProcess: measuringColor,
          noWlan: noWifiColor,
        },
      }),
    });

    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();
    if (data.status === 'ok') {
      alert('Ihre Konfiguration wurde erfolgreich gespeichert!');
    }
  } catch (error) {
    console.error('Error: ' + error);
    alert('Etwas lief schief beim Speichern der Konfiguration');
  }
}

const xValues = [50, 60, 70, 80, 90, 100, 110, 120, 130, 140, 150];
const yValues = [7, 8, 8, 9, 9, 9, 10, 11, 14, 14, 15];

const ctx = document.getElementById('graph').getContext('2d');

new Chart(ctx, {
  type: 'line',
  data: {
    labels: xValues,
    datasets: [
      {
        backgroundColor: 'rgba(0,0,255,1.0)',
        borderColor: 'rgba(0,0,255,0.1)',
        data: yValues,
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
  },
});
