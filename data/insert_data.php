<?php

if (isset($_GET["temperature"]) && isset($_GET["humidity"]) && isset($_GET["timestamp"])) {
  $temperature = $_GET["temperature"];
  $humidity = $_GET["humidity"];
  $timestamp = $_GET["timestamp"];

  $servername = "192.168.0.17";
  $username = "root";
  $password = "root";
  $database_name = "wetterstation";

  $connection = new mysqli($servername, $username, $password, $database_name);
  if ($connection->connect_error) {
    die("MySQL connection failed: " . $connection->connect_error);
  }

  $sql = "INSERT INTO messdaten (temperatur,luftfeuchtigkeit,timestamp) VALUES ($temperature,$humidity,$timestamp)";

  if ($connection->query($sql) === TRUE) {
    echo "New record created successfully";
  } else {
    echo "Error: " . $sql . " => " . $connection->error;
  }

  $connection->close();
} else {
  echo "temperature is not set in the HTTP request";
}