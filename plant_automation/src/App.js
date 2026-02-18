import './TextAndButtons.css';
import './Crt.css';

function App() {
  function alerted(){
    alert("you clicked the button");
  }
  return (

    <div className="vignette">
    <div className="crtDisplay">
        <div className="center">
          <div className="buttonHolder">
            <button className="sampleButton" onClick = {alerted} >Testing button</button>
          </div>
        </div>
  </div>
    </div>
  );
}

export default App;
