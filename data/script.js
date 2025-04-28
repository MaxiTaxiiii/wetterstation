// Sobald die Seite geladen ist, dann fetch die config datei vom ESP32 und aktualisiere die Konfigurationsfläche
window.addEventListener('load', async () => {
  try {
    const response = await fetch(`/api/config`);

    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();

    document.getElementById('interval').value = data.interval;
    document.getElementById('radio-on').checked = data.led;
    document.getElementById('radio-off').checked = !data.led;

    document.getElementById('color-1').value = data.standby;
    document.getElementById('color-2').value = data.sleep;
    document.getElementById('color-3').value = data.highTemperature;
    document.getElementById('color-4').value = data.measurementInProcess;
    document.getElementById('color-5').value = data.noWlan;

    fetchWeatherData();
    setInterval(fetchWeatherData, 30000);
  } catch (error) {
    console.error('Error: ' + error);
  }
});

async function fetchWeatherData() {
  try {
    const response = await fetch('/api/data');

    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();
    document.getElementById('humidity').innerHTML = data.humidity + '%';
    document.getElementById('temperature').innerHTML = data.temperature + 'C';
    updateGraph(new Date(data.timestamp), data.temperature, data.humidity);
  } catch (error) {
    console.error('Error: ' + error);
  }
}

const processChange = debounce(saveChanges, 350);

async function saveChanges(event) {
  try {
    const key = event.target.name;

    // Verarbeite die Values
    let value = null;
    switch (key) {
      case 'interval':
        value = Number(event.target.value);
        break;
      case 'led':
        value = JSON.parse(event.target.value);
        break;
      default:
        value = event.target.value;
    }

    const response = await fetch('/api/config', {
      method: 'post',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        [key]: value,
      }),
    });

    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();
    if (data.status === 'ok') {
      console.log('Speichern war erfolgreich!');
    }
  } catch (error) {
    console.error('Error: ' + error);
  }
}

document.querySelector('button').addEventListener('click', async () => {
  try {
    const response = await fetch('/api/measure');
    if (!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();
    if (data.status === 'ok') {
      console.log('Request war erfolgreich!');
    }
  } catch (error) {
    console.error('Error: ' + error);
  }
});

// füge eventlisteners für alle inputs hinzu
document.querySelectorAll('input').forEach((element) => {
  element.addEventListener('input', () => processChange(event));
});

// füge eventlistener für das dropdown menu
document
  .querySelector('select')
  .addEventListener('change', () => processChange(event));

function debounce(func, timeout) {
  let id;
  return (...args) => {
    clearTimeout(id);
    id = setTimeout(() => {
      func.apply(this, args);
    }, timeout);
  };
}

const timestamps = [];
const temperatureValues = [];
const humidityValues = [];

const ctx = document.getElementById('graph').getContext('2d');

const chart = new Chart(ctx, {
  type: 'line',
  data: {
    labels: timestamps,
    datasets: [
      {
        label: 'Temperature (°C)',
        backgroundColor: 'rgb(255,145,0)',
        borderColor: 'rgb(255,145,0)',
        data: temperatureValues,
        fill: false,
      },
      {
        label: 'Humidity (%)',
        backgroundColor: 'rgb(0, 123, 255)',
        borderColor: 'rgb(0, 123, 255)',
        data: humidityValues,
        fill: false,
      },
    ],
  },
  options: {
    responsive: true,
    maintainAspectRatio: false,
    scales: {
      x: {
        type: 'time',
        time: {
          tooltipFormat: 'yyyy-MM-dd HH:mm',
          displayFormats: {
            millisecond: 'HH:mm',
            second: 'HH:mm',
            minute: 'HH:mm',
            hour: 'HH:mm',
          },
          parser: function (date) {
            return new Date(
              Date.UTC(
                date.getUTCFullYear(),
                date.getUTCMonth(),
                date.getUTCDate(),
                date.getUTCHours() - 2,
                date.getUTCMinutes(),
                date.getUTCSeconds()
              )
            );
          },
        },
        ticks: {
          source: 'auto',
          autoSkip: true,
        },
        title: {
          display: true,
          text: 'Time',
        },
      },
      y: {
        title: {
          display: true,
          text: 'Value',
        },
      },
    },
  },
});

function updateGraph(timestamp, temperature, humidity) {
  const interval = document.getElementById('interval').value;
  if (
    timestamps.length === 0 ||
    timestamp.getTime() - timestamps[timestamps.length - 1].getTime() >=
      interval
  ) {
    timestamps.push(timestamp);
    temperatureValues.push(temperature);
    humidityValues.push(humidity);
    chart.update();
  }
}
