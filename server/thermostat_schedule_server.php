<?php

//declare(strict_types=1);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_get($sql_connection){
//  echo "process_get\n";

  if( $_GET["delete_id"] ) {
    $delete_id = htmlspecialchars($_GET["delete_id"]);
//    echo "got delete_id $delete_id\n"; 

    $result = mysqli_query($sql_connection, "DELETE FROM temp_schedule where id='$delete_id'");
    echo "<h2>Deleted Schedule</h2>\n";
    echo "<h3>Go Back to Return to the Thermostat Configuration Webpage</h3>\n";
  }else {  
  

  $result = mysqli_query($sql_connection, "SELECT * FROM temp_schedule ORDER BY DAY, SET_TIME ASC");
  $rows = array();

  while($r = mysqli_fetch_assoc($result)){
    $rows[] = $r;
  }

  echo json_encode($rows);

//  print json_encode($result_rows);

/*  echo "debug print data";
  $result = mysqli_query($sql_connection, "SELECT * FROM temp_schedule");

  while($query_data = mysqli_fetch_row($result)) {
    echo $query_data[0];
    echo $query_data[1];
    echo $query_data[2];
    echo $query_data[3];
  }*/
  }
  exit();
}//process_get


function process_post($sql_connection){
//  echo "process_post\n";    

  if( $_POST["submit"] ) {
//    echo "got submit\n";  
   
    $day = htmlspecialchars($_POST["DAY"]);
    $time = htmlspecialchars($_POST["TIME"]);
    $temp = htmlspecialchars($_POST["TEMPERATURE"]);

//    echo $day;
//    echo "  |  ";
//    echo $time;
//    echo "  |  ";
//    echo $temp;

    $result = mysqli_query($sql_connection, "INSERT INTO temp_schedule (DAY, SET_TIME, TEMP_SET) VALUES ('$day', '$time', '$temp')");

    $id='1';
    $time_last_programmed = time();

    $result = mysqli_query($sql_connection, "UPDATE status SET TIME_LAST_PROGRAMMED='$time_last_programmed' WHERE ID='$id'");
    
    echo "<h2>New Schedule Set<h2>\n";
    echo "<h3>Go back to return to the Thermostat Configuration Webpage<h3>\n";

     exit();
   }

  exit();
  
}//process_post

function process_delete($sql_connection){
  echo "process_post\n";    

  if( $_POST["submit"] ) {
    echo "got submit\n";  
   
    $day = htmlspecialchars($_POST["DAY"]);
    $time = htmlspecialchars($_POST["TIME"]);
    $temp = htmlspecialchars($_POST["TEMPERATURE"]);

    echo $day;
    echo "  |  ";
    echo $time;
    echo "  |  ";
    echo $temp;

    $result = mysqli_query($sql_connection, "INSERT INTO temp_schedule (DAY, SET_TIME, TEMP_SET) VALUES ('$day', '$time', '$temp')");

    $id='1';
    $time_last_programmed = time();

    $result = mysqli_query($sql_connection, "UPDATE status SET TIME_LAST_PROGRAMMED='$time_last_programmed' WHERE ID='$id'");

     exit();
   }

  exit();
  
}//process_post


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

  $query = "CREATE TABLE IF NOT EXISTS temp_schedule (
        ID INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
        DAY ENUM('MONDAY', 'TUESDAY', 'WEDNESDAY', 'THURSDAY', 'FRIDAY', 'SATURDAY', 'SUNDAY') NOT NULL,
        SET_TIME TIME NOT NULL,
        TEMP_SET INT NOT NULL)";

  if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

//  echo "Table temp_schedule is good";

  return $sql_connection;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_request($sql_connection){
//  echo "process_request\n";
  $method = $_SERVER['REQUEST_METHOD'];
//  echo "Schedule Method = $method \n";

  switch ($method) {
    case 'GET':
      process_get($sql_connection);  
      break;
    case 'POST':
      process_post($sql_connection);
      break;

     case 'DELETE':
     case 'PUT':
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
<h1>ECE 531 Thermostat Schedule</h1>

<!-- Clean up. -->
<?php

//  mysqli_free_result($result);
//  mysqli_close($connection);
  mysqli_free_result($result);
  mysqli_close($sql_connection);


?>

</body>
</html>


