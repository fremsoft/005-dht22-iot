<!DOCTYPE html>
<html>
  <head>
    <title>Sensori DHT22</title>
	<style>td { text-align:center; padding:0 20px; }</style>
  <head>
  <body>
<?php

$NOMEFILE_JSON = "service-dht-data.json";
date_default_timezone_set('Europe/Rome');

if ((isset( $_GET["device"])) && (isset( $_GET["t"])) && (isset( $_GET["h"]))) {
	
	$dati = json_decode( file_get_contents($NOMEFILE_JSON), true );
	
	$dato = array();	
	$dato["timestamp"] = time();
	$dato["t"]         = $_GET["t"];
	$dato["h"]         = $_GET["h"];
	
	$dati[$_GET["device"]] = $dato;	
	
	
	// encode array to json
    $json = json_encode($dati);
	
	//write json to file
	if (file_put_contents( $NOMEFILE_JSON, $json)) {
		echo "<p>JSON file created successfully...</p>";
	    print ( $json );
    } 
	else {
		echo "Oops! Error creating json file...";
	}


} else {
	print ("<p>Ciao, ecco le varie temperature</p>");
	print ("<table>");
    print ("<tr><th>DISPOSITIVO</th><th>ORA</th><th>TEMPERATURA</th><th>UMIDITA'</th></tr>");
	
	$dati = json_decode( file_get_contents($NOMEFILE_JSON), true );
	foreach($dati as $device => $dato) {
		$dataora = date("H:i", $dato['timestamp']);
		print ("<tr><td>{$device}</td><td>{$dataora}</td><td>{$dato['t']}°C</td><td>{$dato['h']}%</td></tr>") ;
	}
	print ("</table>");
	
}

?>  
  </body>
</html>