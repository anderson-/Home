// $(function(){
//   var includes = $('[data-include]');
//   jQuery.each(includes, function(){
//     var file = '/' + $(this).data('include') + '.html';
//     $(this).load(file);
//   });
// });
var ctx = document.getElementById("canvas");

var myLineChart = new Chart(ctx, {
    type: 'line',
    data: {
			datasets: []
    },
    options: {
      animation: {
        duration: 0,
      },
      hover: {
        animationDuration: 0,
      },
      responsiveAnimationDuration: 0,
      elements: {
        line: {
          tension: 0,
        }
      },
			scales: {
				xAxes: [{
					type: 'time',
          time: {
            unit: 'second',
          },
          ticks: {
            autoSkip: true,
            maxTicksLimit: 20
          }
				}],
				yAxes: [{
					scaleLabel: {
						display: true,
						labelString: 'Closing price ($)'
					}
				}]
			}
		}
});

function addData(chart, label, time, value) {
  maxlen = 40;
  while (chart.data.labels.length > maxlen){
    chart.data.labels.splice(0, 1);
  }
  for (i = 0; i < chart.data.datasets.length; i++) {
    dataset = chart.data.datasets[i];
    while (dataset.data.length > maxlen){
      dataset.data.splice(0, 1);
    }
  }
  chart.update();

  chart.data.labels.push(time);
  ok = false;
  for (i = 0; i < chart.data.datasets.length; i++) {
    dataset = chart.data.datasets[i];
    if (dataset.label == label){
      dataset.data.push(value);
      ok = true;
    }
  }
  if (!ok){
    chart.data.datasets.push({
      label: label,
      borderColor: "#3e95cd",
      fill: false,
      borderWidth: 2,
      data: [value]
    });
  }
  chart.update();
}

var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);
connection.onopen = function() {
  connection.send('Connect ' + new Date());
};
connection.onerror = function(error) {
  console.log('WebSocket Error ', error);
};
connection.onmessage = function(e) {
  console.log('Server: ', e.data);
  if (e.data.indexOf("=") !=-1) {
    label = e.data.substring(0, e.data.indexOf("="));
    value = e.data.substring(e.data.indexOf("=") + 1);
    addData(myLineChart, label, moment().toDate(), value);
  }
};

var myVar = setInterval(myTimer, 300);

function myTimer() {
  connection.send('#analogRead 0');
  connection.send('#analogRead 1');
}
