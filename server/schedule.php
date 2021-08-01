<?php
  //handles the main webpage tables/data that must be refreshed frequently to update

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function setup_sql_connection(){
    //echo "Setup Database and Connect\n";

    include "../inc/dbinfo.inc"; 
    $sql_connection = new mysqli(DB_SERVER, DB_USERNAME, DB_PASSWORD);

    if (mysqli_connect_errno()) echo "Failed to connect to MySQL: " . mysqli_connect_error();
    //echo "Connection Successful to MYSQL Database";

    $server_database = mysqli_select_db($sql_connection, DB_DATABASE);

    $query = "CREATE TABLE IF NOT EXISTS temp_schedule (
        ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
        DAY ENUM('MONDAY', 'TUESDAY', 'WEDNESDAY', 'THURSDAY', 'FRIDAY', 'SATURDAY', 'SUNDAY') NOT NULL,
        SET_TIME TIME NOT NULL,
        TEMP_SET INT NOT NULL)";

    if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

    //echo "Table temp_schedule is good";
    $query = "CREATE TABLE IF NOT EXISTS status (
        ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
        TIME_LAST_PROGRAMMED BIGINT UNSIGNED NOT NULL,
        TIME_LAST_UPDATE BIGINT UNSIGNED NOT NULL,
        CURR_TEMP INT NOT NULL,
        SET_TEMP INT NOT NULL,
        POWER INT NOT NULL,
        NEW_TEMP INT)";

    if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

    //echo "Table status is good";

    return $sql_connection;
  }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function get_thermostat_schedule($sql_connection){
    return mysqli_query($sql_connection, "SELECT * FROM temp_schedule ORDER BY DAY, SET_TIME ASC");
  }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function get_status($sql_connection){
    return mysqli_query($sql_connection, "SELECT * FROM status");
  }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_thermostat_program_table($result){
//    echo "print table";
      echo "<h3>Thermostat Schedule</h3>";
      echo "<table border=\"1\" cellpadding=\"2\" cellspacing=\"2\">";
      echo "<tr>";
//      echo "<td>ID</td>";
      echo "<td>DAY</td>";
      echo "<td>TIME</td>";
      echo "<td>TEMPERATURE</td>";
      echo "</tr>";


    while($query_data = mysqli_fetch_array($result)) {
      echo "<tr>";
//      echo "<td>",$query_data[0], "</td>";
      echo "<td>",$query_data[1], "</td>",
           "<td>",$query_data[2], "</td>",
           "<td>",$query_data[3], "</td>";
      echo "<td><a href='thermostat_schedule_server.php?delete_id=",$query_data[0],"'>Delete</a></td>";
      echo "</tr>";
    }
    echo "</table>";
  }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_status_table($result){
//    echo "print table";
      echo "<h3>Status Debug</h3>";
      echo "<table border=\"1\" cellpadding=\"2\" cellspacing=\"2\">";
      echo "<tr>";
      echo "<td>ID</td>";
      echo "<td>TIME_LAST_PROGRAMMED</td>";
      echo "<td>TIME_LAST_UPDATE</td>";
      echo "<td>CURR_TEMP</td>";
      echo "<td>SET_TEMP</td>";
      echo "<td>POWER</td>";
      echo "<td>NEW_TEMP</td>";
      echo "</tr>";

    while($query_data = mysqli_fetch_row($result)) {
      echo "<tr>";
      echo "<td>",$query_data[0], "</td>",
           "<td>",date('F j, Y, g:i:s a T',$query_data[1]), "</td>",
           "<td>",date('F j, Y, g:i:s a T',$query_data[2]), "</td>",
           "<td>",$query_data[3], "</td>",
	   "<td>",$query_data[4], "</td>",
           "<td>",$query_data[5], "</td>",
           "<td>",$query_data[6], "</td>";
      echo "</tr>";
    }
    echo "</table>";

  }

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_current_time(){
    $query_data = mysqli_fetch_row($result);

    echo "<h3>Time</h3>";
    echo date('F j, Y, g:i:s a T');
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_last_updated_timestamp($result){
    $query_data = mysqli_fetch_row($result);

    echo "<h3>Thermostat Time</h3>";
    echo date('F j, Y, g:i:s a T', $query_data[2]);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_current_temp($result){
    $query_data = mysqli_fetch_row($result);

    echo "<h3>Thermostat Temperature : ";
    echo $query_data[3];
    echo "&deg;F</h3>";
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_set_temp($result){
    $query_data = mysqli_fetch_row($result);

    echo "<h3>Thermostat Set Temperature : ";
    echo $query_data[4];
    echo "&deg;F</h3>";
   
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_power($result){
    $query_data = mysqli_fetch_row($result);

    echo "<h3>Current Power : ";
    if($query_data[5])
      echo "ON";
    else
      echo "OFF";
    echo"</h3>\n";
  }

  $sql_connection = setup_sql_connection();
  date_default_timezone_set('America/Denver');

  $result = get_status($sql_connection);
  print_set_temp($result);

$result = get_status($sql_connection);
  print_current_temp($result);

  $result = get_status($sql_connection);
  print_power($result);

//  print_current_time();

  $result = get_status($sql_connection);
  print_last_updated_timestamp($result);

  $result = get_thermostat_schedule($sql_connection);
  print_thermostat_program_table($result);

//  $result = get_status($sql_connection);
//  print_status_table($result);

  mysqli_free_result($result);
  mysqli_close($sql_connection);
?>

