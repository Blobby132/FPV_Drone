namespace FPVSim.Core
{
    /// <summary>
    /// A game mode decides what "respawn" means, when a run starts and ends, and what counts as out of bounds.
    /// v1 only has <see cref="FreeFlyMode"/>. A race mode would implement this interface, subscribe to
    /// <see cref="GameplayEvents.DronePassedTrigger"/> for its gates, and respawn at the last checkpoint.
    /// </summary>
    public interface IGameMode
    {
        string DisplayName { get; }

        /// <summary>Called once by the <see cref="GameSession"/> after everything is wired up.</summary>
        void Begin(GameSession session);

        /// <summary>Called every frame while the game is not paused.</summary>
        void Tick(float deltaTime);

        /// <summary>The pilot pressed the reset button.</summary>
        void OnRespawnRequested();

        /// <summary>Called when the session shuts down or the mode is replaced.</summary>
        void End();
    }
}
