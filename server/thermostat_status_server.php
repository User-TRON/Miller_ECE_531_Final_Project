<?php
//handles status updates and requests

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_get($sql_connection){
  echo "process_get\n";

  $time_last_update = time();
  $result = mysqli_query($sql_connection, "UPDATE status SET TIME_LAST_UPDATE='$time_last_update' WHERE ID='1'");


  $result = mysqli_query($sql_connection, "SELECT * FROM status");
  $rows = array();

  while($r = mysqli_fetch_assoc($result)){
    $rows[] = $r;
  }

  echo json_encode($rows);

  exit();
}//process_get

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_put($sql_connection){
//  echo "process_put\n";    

  if( $_GET["id"] ) {
    echo "got id\n";
    $id = htmlspecialchars($_GET["id"]);

    $time_last_update = time();
    $curr_temp = htmlspecialchars($_GET["curr_temp"]);
    $set_temp = htmlspecialchars($_GET["set_temp"]);
    $power = htmlspecialchars($_GET["power"]);

    //save to mysql
    $result = mysqli_query($sql_connection, "UPDATE status SET TIME_LAST_UPDATE='$time_last_update', CURR_TEMP='$curr_temp', SET_TEMP='$set_temp', POWER='$power', NEW_TEMP=NULL WHERE ID='$id'");
    exit();
  }
  else if($_POST["new_temp"]){
//    echo "got new_temp\n";

    $id='1';
    $time_last_programmed = time();
    $new_temp = htmlspecialchars($_POST["TEMPERATURE"]);
//    echo $time_last_programmed;
//    echo $new_temp;
    
    if($new_temp != null){
      $result = mysqli_query($sql_connection, "UPDATE status SET TIME_LAST_PROGRAMMED='$time_last_programmed', NEW_TEMP='$new_temp' WHERE ID='$id'");
      echo "<h2>Set temperature to ";
      echo $new_temp;
      echo "&deg;F<h2>\n";
      echo "<h3>Go back to return to the Thermostat Configuration Webpage<h3>\n";
      exit();
    }
  } 
}//process_put

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function handle_error(){
  echo "Invalid request\n";
  exit();
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function setup_sql_connection(){
//  echo "Setup Database and Connect\n";

  include "../inc/dbinfo.inc"; 
  $sql_connection = new mysqli(DB_SERVER, DB_USERNAME, DB_PASSWORD);

  if (mysqli_connect_errno()) echo "Failed to connect to MySQL: " . mysqli_connect_error();
//  echo "Connection Successful to MYSQL Database";

  $server_database = mysqli_select_db($sql_connection, DB_DATABASE);

  $query = "CREATE TABLE IF NOT EXISTS status (
        ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
        TIME_LAST_PROGRAMMED BIGINT UNSIGNED NOT NULL,
        TIME_LAST_UPDATE BIGINT UNSIGNED NOT NULL,
        CURR_TEMP INT NOT NULL,
	SET_TEMP INT NOT NULL,
	POWER INT NOT NULL,
        NEW_TEMP INT)";

  if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

//  echo "Table status is good";

  return $sql_connection;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_request($sql_connection){
//  echo "process_request\n";
  $method = $_SERVER['REQUEST_METHOD'];
//  echo "Status Method = $method \n";

  switch ($method) {
    case 'POST':
    case 'PUT':
      process_put($sql_connection);  
      break;

    case 'GET':
      process_get($sql_connection);  
      break;

    case 'DELETE':

    default:
      handle_error();  
      break;
  }
}

$sql_connection = setup_sql_connection();
process_request($sql_connection);

?>


<!DOCTYPE html>
<html>
<body>
<h1>ECE 531 Thermostat Control</h1>

<!-- Clean up. -->
<?php

  mysqli_free_result($result);
  mysqli_close($sql_connection);

?>

</body>
</html>

