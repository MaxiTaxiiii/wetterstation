
function convertMs(ms) {
  const sec = 1000;
  const min = sec * 60;
  const hour = min * 60;
  const day = hour * 24;

  return {
      days: Math.floor(ms / day),
      hours: Math.floor((ms % day) / hour),
      minutes: Math.floor((ms % hour) / min),
      seconds: Math.floor((ms % min) / sec),
      milliseconds: ms % 1000
  };
}

window.onload = async () => {
  const ip = window.location.hostname;
  try {
    const response = await fetch(`http://${ip}/api/config`);

    if(!response.ok) {
      throw new Error('Network response was not ok');
    }

    const data = await response.json();

    document.getElementById('radio-1').checked = data.ledOn;
    document.getElementById('radio-2').checked = !data.ledOn;

    const intervalElement = document.getElementById('messvorgang-intervall');
    const interval = convertMs(data.interval);

    if(interval.hours === 2) {
      intervalElement.value = 'stunde-2';
    } else if(interval.hours === 1) {
      intervalElement.value = 'stunde-1';
    } else if(interval.minutes === 30) {
      intervalElement.value = 'minute-30';
    } else if(interval.minutes === 15) {
      intervalElement.value = 'minute-15';
    }

    document.getElementById('color-1').value = data.standby;
    document.getElementById('color-2').value = data.sleep;
    document.getElementById('color-3').value = data.highTemperature;
    document.getElementById('color-4').value = data.measurementInProcess;
    document.getElementById('color-5').value = data.noWlan;

  } catch(error) {
    console.error('Error: ' + error);
  }
}

async function saveConfig(event) {
  event.preventDefault();
  const intervalValue = document.getElementById('messvorgang-intervall').value;
  let intervalMillis = 0;
  switch(intervalValue) {
    case 'stunde-2':
      intervalMillis = 7200000;
      break;
    case 'stunde-1':
      intervalMillis = 3600000;
      break;
    case 'minute-30':
      intervalMillis = 1800000;
      break;
    case 'minute-15':
      intervalMillis = 900000;
      break;
  }
      
  const ledPower = document.getElementById('radio-1').checked;
  const standbyColor = document.getElementById('color-1').value;
  const sleepColor = document.getElementById('color-2').value;
  const highTempColor = document.getElementById('color-3').value;
  const measuringColor = document.getElementById('color-4').value;
  const noWifiColor = document.getElementById('color-5').value;

  const ip = window.location.hostname;

  try {
    const response = await fetch(`http://${ip}/api/config`, {
      method: 'post',
      headers: {
        'Content-Type': 'application/json',
      },
      body: JSON.stringify({
        ledOn: ledPower,
        interval: intervalMillis,
        standby: standbyColor,
        sleep: sleepColor,
        highTemperature: highTempColor,
        measurementInProcess: measuringColor,
        noWlan: noWifiColor,
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
