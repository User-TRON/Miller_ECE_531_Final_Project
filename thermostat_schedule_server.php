<?php

//declare(strict_types=1);

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_get($sql_connection){
//  echo "process_get\n";

  if( $_GET["id"] ) {
      $id = htmlspecialchars($_GET["id"]);
//    echo "id = $id \n";
//    echo "result = ";
    $result = mysqli_query($sql_connection, "SELECT * FROM http_server WHERE id = $id");

    while($query_data = mysqli_fetch_row($result)) {
      echo json_encode(unserialize($query_data[1]));
    }
    echo "\n";

    exit();
  }

}//process_get

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_post($sql_connection){
//  echo "process_post\n";    
  $data = json_decode(file_get_contents('php://input'),true);
//  print_r($data);

//  echo "serialize json data\n";
  $serial_data = serialize($data);
//  print_r($serial_data);

  //save to mysql
  $result = mysqli_query($sql_connection, "INSERT INTO http_server (json) VALUES ('$serial_data')");

  //return id

  $id = mysqli_insert_id($sql_connection);
//  echo "New id =";
  echo json_encode($id);
  echo "\n";
  exit();
}//process_post

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_put($sql_connection){
//  echo "process_put\n";    

  if( $_GET["id"] ) {
      $id = htmlspecialchars($_GET["id"]);
//    echo "id = $id \n";
      
  $data = json_decode(file_get_contents('php://input'),true);
//  print_r($data);

//  echo "serialize json data\n";
  $serial_data = serialize($data);
//  print_r($serial_data);

  //save to mysql
  $result = mysqli_query($sql_connection, "REPLACE INTO http_server (id,json) VALUES ($id,'$serial_data')");
    exit();
  }

  exit();
}//process_put

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_delete($sql_connection){
//  echo "process_delete\n";    

  if( $_GET["id"] ) {
    $id = htmlspecialchars($_GET["id"]);
//    echo "id = $id \n";
//    echo "result = ";
    $result = mysqli_query($sql_connection, "DELETE FROM http_server WHERE id = $id");

    exit();
  }

}//process_delete


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

  $query = "CREATE TABLE IF NOT EXISTS http_server (
      id INT NOT NULL PRIMARY KEY AUTO_INCREMENT, 
      json VARCHAR(1000) NOT NULL)";

  if(!mysqli_query($sql_connection, $query)) echo("<p>Error creating table.</p>");

//  echo "Table is good";

  return $sql_connection;
}

//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function process_request($sql_connection){
//  echo "process_request\n";
  $method = $_SERVER['REQUEST_METHOD'];
//  echo "Method = $method \n";

  switch ($method) {
    case 'PUT':
      process_put($sql_connection);  
      break;

    case 'POST':
      process_post($sql_connection);  
      break;

    case 'GET':
      process_get($sql_connection);  
      break;

    case 'DELETE':
      process_delete($sql_connection);
      break;

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

<?php
//~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
function print_thermostat_program_table(){
  $result = mysqli_query($sql_connection, "SELECT * FROM thermostat_server");

  while($query_data = mysqli_fetch_row($result)) {
    echo "<tr>";
    echo "<td>",$query_data[1], "</td>",
         "<td>",$query_data[2], "</td>",
         "<td>",$query_data[3], "</td?";
    echo "</tr>";
  }
}
  /* If input fields are populated, add a row to the EMPLOYEES table. */
  $employee_name = htmlentities($_POST['id']);
  $employee_address = htmlentities($_POST['json']);

  if (strlen($employee_name) || strlen($employee_address)) {
//    AddEmployee($connection, $employee_name, $employee_address);
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_last_updated_timestamp(){
   echo "timestamp here\n";
  }

  //~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
  function print_current_temp(){
    echo "current temp here\n";
  }

?>

<!-- Input form -->
<form action="<?PHP echo $_SERVER['SCRIPT_NAME'] ?>" method="PUT">
  <table border="0">
    <tr>
      <td>ID</td>
      <td>JSON</td>
    </tr>
    <tr>
      <td>
        <input type="text" name="id" maxlength="45" size="30" />
      </td>
      <td>
        <input type="text" name="json" maxlength="90" size="60" />
      </td>
      <td>
        <input type="submit" value="Add Data" />
      </td>
    </tr>
  </table>
</form>

<?php
  print_last_updated_timestamp();
  print_current_temp();
?>


<!-- Display table data. -->
<table border="1" cellpadding="2" cellspacing="2">
  <tr>
    <td>DAY</td>
    <td>TIME</td>
    <td>TEMPERATURE</td>
  </tr>



<?php

  print_thermostat_program_table();

?>

</table>

<!-- Clean up. -->
<?php

//  mysqli_free_result($result);
//  mysqli_close($connection);
  mysqli_free_result($result);
  mysqli_close($sql_connection);


?>

</body>
</html>


<?php

/* Add an employee to the table. */
//function AddEmployee($connection, $name, $address) {
//   $n = mysqli_real_escape_string($connection, $name);
//   $a = mysqli_real_escape_string($connection, $address);

//   $query = "INSERT INTO EMPLOYEES (NAME, ADDRESS) VALUES ('$n', '$a');";

//   if(!mysqli_query($connection, $query)) echo("<p>Error adding employee data.</p>");
//}

?>
