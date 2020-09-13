graphs = {};
t0 = undefined;
vcount = undefined;
max_size = 200;

window.onload = function() {
  var connection = new WebSocket('ws://' + location.hostname + ':81/', ['arduino']);

  function addCmdButton(name, cmd){
    var button = document.createElement('button');
    button.innerHTML = name;
    button.onclick = function(){
      connection.send(cmd);
      return false;
    };
    document.body.appendChild(button);
  }

  addCmdButton('report', '#report');
  addCmdButton('up 5000', '#update report 5000');
  addCmdButton('up 2000', '#update report 2000');
  addCmdButton('up 1000', '#update report 1000');
  addCmdButton('up 500', '#update report 500');
  addCmdButton('led', '#digitalWrite 16 2');
  addCmdButton('relay', '#digitalWrite 17 2');
  addCmdButton('reset', '!');
  addCmdButton('noSerial', '-');
  addCmdButton('Serial', '+');
  addCmdButton('Serial', '#bb ?');
  addCmdButton('0', '#bb B M 0');
  addCmdButton('1', '#bb B M 1');
  addCmdButton('2', '#bb B M 2');
  addCmdButton('3', '#bb B M 3');
  addCmdButton('4', '#bb B M 4');
  addCmdButton('5', '#bb B M 5');
  addCmdButton('6', '#bb B M 6');
  addCmdButton('7', '#bb B M 7');
  addCmdButton('8', '#bb B M 8');
  addCmdButton('light on', '#ctrl L 1');
  addCmdButton('light off', '#ctrl L 0');
  addCmdButton('r200', '#ctrl R 200');
  addCmdButton('r100', '#ctrl R 100');
  addCmdButton('r0', '#ctrl R 0');
  // addCmdButton('Serial', '#bb T 0000000000 ');

  function map (num, in_min, in_max, out_min, out_max) {
    return (num - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
  }

  function addPoindData(graph, time, value) {
    if (Array.isArray(value)){
      graph.data.push([time].concat(value));
    } else {
      graph.data.push([time, value]);
    }
    while (graph.data.length > max_size){
      graph.data.shift();
    }
    graph.g.updateOptions({'file': graph.data});
  }

  function plotData(label, time, value) {
    // console.log(label);
    // console.log(time);
    // console.log(value);
    var labels = undefined;
    if (Array.isArray(label)){
      labels = label;
      label = 'all';
      // value[0] = map(value[0],0,1024,0,1024);
      value[1] = map(value[1],0,200,0,1024);
      value[2] = map(value[2],0,200,0,1024);
      value[3] = map(value[3],0,100,0,1024);
      value[4] = map(value[4],0,1,0,1024);
      value[5] = map(value[5],0,1,0,1024);
    }
    label = label.replace(/\W/g, '')
    var graph = graphs[label];
    if (graph){
      addPoindData(graph, time, value);
    } else {
      var divname = 'div_' + label;
      div = document.getElementById(divname);
      if (!div) {
        var div = document.createElement("div");
        div.id = divname;
        document.body.appendChild(div);
      }
      var data = [];
      if (!labels){
        labels = ['Time', label];
        data.push([time, value]);
      } else {
        data.push([time].concat(value));
      }
      console.log(labels);
      console.log(data);
      graphs[label] = {
        'data': data,
        'g': new Dygraph(document.getElementById(divname), data, {
          drawPoints: false,
          showRoller: true,
          rollPeriod: 2,
          legend: 'always',
          animatedZooms: true,
          labels: labels
        })
      };
    }
  }

  connection.onopen = function() {
    connection.send('Connect ' + new Date());
    // connection.send('!');
  };
  connection.onerror = function(error) {
    console.log('WebSocket Error ', error);
  };
  connection.onmessage = function(e) {
    if (e.data.indexOf("=") !=-1) {
      var time = 0;
      var all = e.data.split(";");
      var values = [];
      var labels = [];
      if (!vcount){
        vcount = all.length;
      } else if (all.length != vcount) {
        console.log('warning invalid data received: ' + e.data);
        return;
      }
      for (i = 0; i < all.length; i++){
        label = all[i].substring(0, all[i].indexOf("="));
        labels.push(label);
        value = Number(all[i].substring(all[i].indexOf("=") + 1));
        if (i == 0){
          if (label === "time"){
            if (!t0){
              t0 = new Date();
            }
            time = new Date(t0.getTime() + value);
            if (isNaN(time)){
              console.log('warning invalid time value received: ' + e.data);
              return;
            }
          } else {
            console.log('warning invalid time label received: ' + e.data);
            return;
          }
        } else {
          values.push(value);
          plotData(label, time, value);
        }
        // console.log(label + ': ' + value);
      }
      plotData(labels, time, values);
    } else if (e.data.indexOf("Ready") !=-1) {
      console.log('Server: [', e.data, ']');
      connection.send('#update report 1000');
    } else {
      console.log('Server: [', e.data, ']');
    }
  };

  // var myVar = setInterval(myTimer, 300);
  //
  // function myTimer() {
  //   connection.send('#analogRead 0');
  //   connection.send('#analogRead 1');
  // }
};
