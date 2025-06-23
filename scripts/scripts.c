// scripts.c (in $profile:scripts)
bool g_ModuleInitialized = false; // Globale Instanzen
ref BunkerManager g_BunkerManager;
ref EXTRACT_LootManager g_LootManager; // Klasse zur Verwaltung von Bunkern
class BunkerManager
{
    protected ref map<int, EXTRACT_BunkerEntity> m_PlayerBunkers = new map<int, EXTRACT_BunkerEntity>(); // Spieler-ID -> Bunker
    protected ref map<int, vector> m_PlayerOriginalPositions = new map<int, vector>(); // Speichert Ursprungspositionen der Spieler
    protected ref array<EXTRACT_BunkerEntity> m_AllBunkers = new array<EXTRACT_BunkerEntity>(); // Liste aller Bunker

    void BunkerManager()
    {
        Print("BunkerManager: Initialized.", LogLevel.NORMAL);
    }

    EXTRACT_BunkerEntity CreateSingleBunkerForPlayer(int playerId)
    {
        if (playerId <= 0)
        {
            Print("BunkerManager: Invalid player ID!", LogLevel.ERROR);
            return null;
        }

        EXTRACT_BunkerEntity existingBunker = m_PlayerBunkers.Get(playerId);
        if (existingBunker)
        {
            PrintFormat("BunkerManager: Player %1 already has a bunker.", playerId, LogLevel.WARNING);
            return existingBunker;
        }

        ResourceName bunkerPrefab = "{DABA9815578959F1}Prefabs/Bunker/Bunker.et";
        Resource prefabResource = Resource.Load(bunkerPrefab);
        if (!prefabResource || !prefabResource.IsValid())
        {
            Print("BunkerManager: Invalid or missing bunker prefab!", LogLevel.ERROR);
            return null;
        }

        vector spawnPos = Vector(playerId * 1000, 0, playerId * 1000);
        World world = GetGame().GetWorld();
        if (!world)
        {
            Print("BunkerManager: World is null!", LogLevel.ERROR);
            return null;
        }

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = spawnPos;
        spawnParams.Transform[0] = Vector(1, 0, 0);
        spawnParams.Transform[1] = Vector(0, 1, 0);
        spawnParams.Transform[2] = Vector(0, 0, 1);

        IEntity spawnedEntity = GetGame().SpawnEntityPrefab(prefabResource, world, spawnParams);
        if (!spawnedEntity)
        {
            Print("BunkerManager: Failed to spawn bunker entity!", LogLevel.ERROR);
            return null;
        }

        EXTRACT_BunkerEntity bunker = EXTRACT_BunkerEntity.Cast(spawnedEntity);
        if (!bunker)
        {
            Print("BunkerManager: Failed to cast to EXTRACT_BunkerEntity!", LogLevel.ERROR);
            SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
            return null;
        }

        string playerUuid = EPF_Utils.GetPlayerUID(playerId);
        if (!playerUuid || playerUuid.IsEmpty())
        {
            PrintFormat("BunkerManager: Failed to get player UUID for player %1! Using fallback UUID.", playerId, LogLevel.WARNING);
            playerUuid = "fallback_" + playerId.ToString(); // Fallback UUID
        }

        PrintFormat("BunkerManager: Bunker created successfully for player %1 at position %2.", playerId, spawnPos.ToString(), LogLevel.NORMAL);
        bunker.Load(playerUuid); // void-Methode, keine Bedingung

        EPF_PersistenceComponent persistenceComp = EPF_PersistenceComponent.Cast(bunker.FindComponent(EPF_PersistenceComponent));
        if (!persistenceComp)
        {
            Print("BunkerManager: Bunker has no EPF_PersistenceComponent! Persistence will not work.", LogLevel.WARNING);
        }

        // Weise dem Bunker die Spieler-ID zu
        bunker.SetPlayerId(playerId);

        // Speichere den Bunker in der Zuordnung und Liste
        m_PlayerBunkers.Set(playerId, bunker);
        m_AllBunkers.Insert(bunker);

        // Speichere die Ursprungsposition des Spielers (als Backup)
        IEntity playerEntity = GetGame().GetPlayerManager().GetPlayerControlledEntity(playerId);
        if (playerEntity)
        {
            vector originalPos = playerEntity.GetOrigin();
            m_PlayerOriginalPositions.Set(playerId, originalPos);
            PrintFormat("BunkerManager: Stored original position for player %1: %2", playerId, originalPos.ToString(), LogLevel.NORMAL);
        }
        else
        {
            // Fallback-Position
            m_PlayerOriginalPositions.Set(playerId, Vector(100, 0, 100));
            PrintFormat("BunkerManager: No player entity found for player %1, using fallback position (100,0,100)", playerId, LogLevel.WARNING);
        }

        // Speichere den Bunker in der Datenbank
        Print("BunkerManager: Attempting to save bunker to database...", LogLevel.NORMAL);
        if (!SaveBunkerToDatabase(bunker, playerId, playerUuid))
        {
            Print("BunkerManager: Failed to save bunker to database! Continuing anyway.", LogLevel.WARNING);
        }

        return bunker;
    }

    EXTRACT_BunkerEntity GetBunker(int playerId)
    {
        EXTRACT_BunkerEntity bunker = m_PlayerBunkers.Get(playerId);
        if (bunker)
        {
            PrintFormat("BunkerManager: Found bunker for player %1.", playerId, LogLevel.NORMAL);
        }
        else
        {
            PrintFormat("BunkerManager: No bunker found for player %1.", playerId, LogLevel.NORMAL);
        }
        return bunker;
    }

    EXTRACT_BunkerEntity CreateBunker()
    {
        Print("BunkerManager: Creating generic bunker...", LogLevel.NORMAL);
        ResourceName bunkerPrefab = "{DABA9815578959F1}Prefabs/Bunker/Bunker.et";
        Resource prefabResource = Resource.Load(bunkerPrefab);
        if (!prefabResource || !prefabResource.IsValid())
        {
            Print("BunkerManager: Invalid or missing bunker prefab!", LogLevel.ERROR);
            return null;
        }

        World world = GetGame().GetWorld();
        if (!world)
        {
            Print("BunkerManager: World is null!", LogLevel.ERROR);
            return null;
        }

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = Vector(0, 0, 0);
        spawnParams.Transform[0] = Vector(1, 0, 0);
        spawnParams.Transform[1] = Vector(0, 1, 0);
        spawnParams.Transform[2] = Vector(0, 0, 1);

        IEntity spawnedEntity = GetGame().SpawnEntityPrefab(prefabResource, world, spawnParams);
        if (!spawnedEntity)
        {
            Print("BunkerManager: Failed to spawn bunker entity!", LogLevel.ERROR);
            return null;
        }

        EXTRACT_BunkerEntity bunker = EXTRACT_BunkerEntity.Cast(spawnedEntity);
        if (!bunker)
        {
            Print("BunkerManager: Failed to cast to EXTRACT_BunkerEntity!", LogLevel.ERROR);
            SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
            return null;
        }

        EPF_PersistenceComponent persistenceComp = EPF_PersistenceComponent.Cast(bunker.FindComponent(EPF_PersistenceComponent));
        if (!persistenceComp)
        {
            Print("BunkerManager: Generic bunker has no EPF_PersistenceComponent! Persistence will not work.", LogLevel.WARNING);
        }

        m_AllBunkers.Insert(bunker);
        return bunker;
    }

    void DelayedDeleteBunker(IEntity bunkerEntity)
    {
        if (bunkerEntity)
        {
            SCR_EntityHelper.DeleteEntityAndChildren(bunkerEntity);
            Print("BunkerManager: Bunker deleted after delay.", LogLevel.NORMAL);
        }
    }

    bool RemoveBunker(EXTRACT_BunkerEntity bunkerEntity)
    {
        Print("BunkerManager: Removing bunker...", LogLevel.NORMAL);
        if (bunkerEntity)
        {
            bunkerEntity.Cleanup();
            Print("BunkerManager: Cleanup called to save bunker contents.", LogLevel.NORMAL);

            // Entferne aus der Datenbank
            int playerId = bunkerEntity.GetOwnerId();
            string playerUuid = EPF_Utils.GetPlayerUID(playerId);
            if (!playerUuid.IsEmpty())
            {
                RemoveBunkerFromDatabase(playerUuid);
            }

            GetGame().GetCallqueue().CallLater(DelayedDeleteBunker, 1000, false, bunkerEntity);
            Print("BunkerManager: Scheduled bunker deletion after 1-second delay.", LogLevel.NORMAL);

            foreach (int pId, EXTRACT_BunkerEntity bunker : m_PlayerBunkers)
            {
                if (bunker == bunkerEntity)
                {
                    m_PlayerBunkers.Remove(pId);
                    m_PlayerOriginalPositions.Remove(pId);
                    m_AllBunkers.RemoveItem(bunker);
                    break;
                }
            }
            return true;
        }
        Print("BunkerManager: Invalid bunker entity.", LogLevel.WARNING);
        return false;
    }

    bool RemoveBunkerForPlayer(int playerId)
    {
        PrintFormat("BunkerManager: Removing bunker for player %1", playerId, LogLevel.NORMAL);
        EXTRACT_BunkerEntity bunker = GetBunker(playerId);
        if (bunker)
        {
            return RemoveBunker(bunker);
        }
        Print("BunkerManager: No bunker found for player to remove.", LogLevel.WARNING);
        return false;
    }

    void OnGameStart()
    {
        Print("BunkerManager: OnGameStart called.", LogLevel.NORMAL);
        LoadBunkersFromDatabase();
    }

    void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
    {
        PrintFormat("BunkerManager: Player %1 disconnected. Cause: %2, Timeout: %3", playerId, cause, timeout, LogLevel.NORMAL);
        EXTRACT_BunkerEntity bunker = GetBunker(playerId);
        if (bunker)
        {
            bunker.Cleanup();
            PrintFormat("BunkerManager: Cleanup called for player %1's bunker on disconnect.", playerId, LogLevel.NORMAL);
            GetGame().GetCallqueue().CallLater(RemoveBunkerForPlayer, 1000, false, playerId);
            PrintFormat("BunkerManager: Scheduled bunker removal for player %1 after 1-second delay.", playerId, LogLevel.NORMAL);
        }
    }

    void OnPlayerAuditFail(int playerId)
    {
        PrintFormat("BunkerManager: Player %1 failed audit.", playerId, LogLevel.NORMAL);
    }

    void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, Instigator killer)
    {
        PrintFormat("BunkerManager: Player %1 killed by %2.", playerId, killer.ToString(), LogLevel.NORMAL);

        string victimName = "(BOT) SCAV";
        string killerName = "Unknown";
        string message = "";

        victimName = GetGame().GetPlayerManager().GetPlayerName(playerId);
        if (!victimName || victimName == "")
        {
            victimName = "(BOT) SCAV";
        }

        if (!killerEntity && !killer)
        {
            killerName = "Self/Environment";
            message = string.Format("System💀: **%1** died tragically.", victimName);
        }
        else if (killerEntity)
        {
            int killerPlayerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(killerEntity);
            if (killerPlayerId > 0)
            {
                killerName = GetGame().GetPlayerManager().GetPlayerName(killerPlayerId);
                if (!killerName || killerName == "")
                {
                    killerName = "(BOT) SCAV";
                }
                message = string.Format("Player Kill⚔️: **%1** killed **%2**", killerName, victimName);
            }
            else
            {
                killerName = "AI";
                message = string.Format("AI Kill🤖: **%1** was killed by a SCAV", victimName);
            }
        }
        else if (killer)
        {
            killerName = killer.ToString();
            message = string.Format("💥System💀: **%2** died tragically.", killerName, victimName);
        }

        if (message != "")
        {
            SendToDiscord(message);
        }
        else
        {
            Print("BunkerManager: Failed to generate killfeed message!", LogLevel.WARNING);
        }
    }

    protected void SendToDiscord(string message)
    {
        EXTRACT_Config config = EXTRACT_Config.GetInstance();
        if (!config)
        {
            Print("BunkerManager: Failed to get EXTRACT_Config instance!", LogLevel.ERROR);
            return;
        }

        string webhookUrl = config.GetDiscordKillfeedWebhook();
        if (!webhookUrl || webhookUrl == "")
        {
            Print("BunkerManager: Discord Killfeed Webhook URL is empty!", LogLevel.ERROR);
            return;
        }

        PrintFormat("BunkerManager: Sending killfeed to Discord: %1", message, LogLevel.NORMAL);
        EXTRACT_Discord.SendMessage(webhookUrl, message);
    }

    void RegisterPlayerCharacter(SCR_ChimeraCharacter character)
    {
        Print("BunkerManager: Registering player character...", LogLevel.NORMAL);
    }

    void UnregisterPlayerCharacter(SCR_ChimeraCharacter character)
    {
        Print("BunkerManager: Unregistering player character...", LogLevel.NORMAL);
    }

    void PlayerTeleported(SCR_ChimeraCharacter character)
    {
        Print("BunkerManager: PlayerTeleported called.", LogLevel.NORMAL);
        if (!character)
        {
            Print("BunkerManager: PlayerTeleported: Character is null!", LogLevel.ERROR);
            return;
        }

        int playerId = GetGame().GetPlayerManager().GetPlayerIdFromControlledEntity(character);
        if (playerId <= 0)
        {
            Print("BunkerManager: PlayerTeleported: Invalid player ID!", LogLevel.ERROR);
            return;
        }

        EXTRACT_BunkerEntity bunker = GetBunker(playerId);
        if (!bunker)
        {
            PrintFormat("BunkerManager: PlayerTeleported: No bunker found for player %1!", playerId, LogLevel.WARNING);
            return;
        }

        PrintFormat("BunkerManager: PlayerTeleported for player %1, proceeding with teleport logic.", playerId, LogLevel.NORMAL);

        vector exitOrigin = bunker.GetExitOrigin();
        vector exitAngles = bunker.GetExitAngles();

        if (exitOrigin != vector.Zero)
        {
            PrintFormat("BunkerManager: Teleporting player %1 to bunker exit origin: %2, angles: %3", playerId, exitOrigin.ToString(), exitAngles.ToString(), LogLevel.NORMAL);
            character.SetOrigin(exitOrigin);
            character.SetAngles(exitAngles);
        }
        else
        {
            vector originalPos = m_PlayerOriginalPositions.Get(playerId);
            if (originalPos != vector.Zero)
            {
                PrintFormat("BunkerManager: No valid exit origin found, teleporting player %1 to original position: %2", playerId, originalPos.ToString(), LogLevel.WARNING);
                character.SetOrigin(originalPos);
            }
            else
            {
                vector fallbackPos = Vector(100, 0, 100);
                PrintFormat("BunkerManager: No valid exit origin or original position found for player %1, using fallback position: %2", playerId, fallbackPos.ToString(), LogLevel.WARNING);
                character.SetOrigin(fallbackPos);
            }
        }
    }

    void Unload()
    {
        Print("BunkerManager: Unloading...", LogLevel.NORMAL);
        SaveBunkersToDatabase();
        foreach (int playerId, EXTRACT_BunkerEntity bunker : m_PlayerBunkers)
        {
            if (bunker)
            {
                bunker.Cleanup();
                PrintFormat("BunkerManager: Cleanup called for player %1's bunker during unload.", playerId, LogLevel.NORMAL);
                GetGame().GetCallqueue().CallLater(DelayedDeleteBunker, 1000, false, bunker);
                PrintFormat("BunkerManager: Scheduled bunker deletion for player %1 after 1-second delay.", playerId, LogLevel.NORMAL);
            }
        }
        m_PlayerBunkers.Clear();
        m_PlayerOriginalPositions.Clear();
        m_AllBunkers.Clear();
    }

    // Persistenz-Methoden
    private void LoadBunkersFromDatabase()
    {
        Print("BunkerManager: Attempting to load bunkers from database...", LogLevel.NORMAL);
        EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
        if (!persistenceManager)
        {
            Print("BunkerManager: Persistence manager not found. Cannot load bunkers.", LogLevel.ERROR);
            return;
        }

        array<ref EDF_DbEntity> bunkerData = persistenceManager
            .GetDbContext()
            .FindAll(EXTRACT_BunkerSaveData)
            .GetEntities();

        if (!bunkerData || bunkerData.IsEmpty())
        {
            Print("BunkerManager: No bunker data found in database.", LogLevel.NORMAL);
            return;
        }

        foreach (EDF_DbEntity data : bunkerData)
        {
            EXTRACT_BunkerSaveData saveData = EXTRACT_BunkerSaveData.Cast(data);
            if (saveData)
            {
                EXTRACT_BunkerEntity bunker = CreateBunkerAtPosition(saveData.m_vPosition);
                if (bunker)
                {
                    bunker.SetPlayerId(saveData.m_iPlayerId);
                    bunker.Load(saveData.m_sPlayerUuid);
                    m_PlayerBunkers.Set(saveData.m_iPlayerId, bunker);
                    m_AllBunkers.Insert(bunker);
                    m_PlayerOriginalPositions.Set(saveData.m_iPlayerId, saveData.m_vPosition);
                    PrintFormat("BunkerManager: Loaded bunker for player %1 from database at position %2.", saveData.m_iPlayerId, saveData.m_vPosition.ToString(), LogLevel.NORMAL);
                }
            }
        }
    }

    private bool SaveBunkerToDatabase(EXTRACT_BunkerEntity bunker, int playerId, string playerUuid)
    {
        Print("BunkerManager: Saving bunker to database for player %1...", LogLevel.NORMAL);
        EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
        if (!persistenceManager)
        {
            Print("BunkerManager: Persistence manager not found. Cannot save bunker.", LogLevel.ERROR);
            return false;
        }

        EDF_DbContext dbContext = persistenceManager.GetDbContext();
        if (!dbContext)
        {
            Print("BunkerManager: Database context not found. Cannot save bunker.", LogLevel.ERROR);
            return false;
        }

        EXTRACT_BunkerSaveData saveData = new EXTRACT_BunkerSaveData();
        saveData.m_iPlayerId = playerId;
        saveData.m_sPlayerUuid = playerUuid;
        saveData.m_vPosition = bunker.GetOrigin();

        bool success = dbContext.AddOrUpdate(saveData);
        if (!success)
        {
            Print("BunkerManager: Failed to add or update bunker data in database!", LogLevel.ERROR);
            return false;
        }

        PrintFormat("BunkerManager: Successfully saved bunker for player %1 to database.", playerId, LogLevel.NORMAL);
        return true;
    }

    private void SaveBunkersToDatabase()
    {
        Print("BunkerManager: Saving all bunkers to database...", LogLevel.NORMAL);
        foreach (EXTRACT_BunkerEntity bunker : m_AllBunkers)
        {
            int playerId = bunker.GetOwnerId();
            string playerUuid = EPF_Utils.GetPlayerUID(playerId);
            if (!playerUuid.IsEmpty())
            {
                SaveBunkerToDatabase(bunker, playerId, playerUuid);
            }
        }
    }

    private void RemoveBunkerFromDatabase(string playerUuid)
    {
        PrintFormat("BunkerManager: Attempting to remove bunker for UUID %1 from database...", playerUuid, LogLevel.NORMAL);
        EPF_PersistenceManager persistenceManager = EPF_PersistenceManager.GetInstance();
        if (!persistenceManager)
        {
            Print("BunkerManager: Persistence manager not found. Cannot remove bunker.", LogLevel.ERROR);
            return;
        }

        EDF_DbContext dbContext = persistenceManager.GetDbContext();
        if (!dbContext)
        {
            Print("BunkerManager: Database context not found. Cannot remove bunker.", LogLevel.ERROR);
            return;
        }

        bool success = dbContext.Remove(EXTRACT_BunkerSaveData, playerUuid);
        if (!success)
        {
            PrintFormat("BunkerManager: Failed to remove bunker for UUID %1 from database.", playerUuid, LogLevel.ERROR);
            return;
        }

        PrintFormat("BunkerManager: Successfully removed bunker for UUID %1 from database.", playerUuid, LogLevel.NORMAL);
    }

    private EXTRACT_BunkerEntity CreateBunkerAtPosition(vector position)
    {
        PrintFormat("BunkerManager: Creating bunker at position %1...", position.ToString(), LogLevel.NORMAL);

        ResourceName bunkerPrefab = "{DABA9815578959F1}Prefabs/Bunker/Bunker.et";
        Resource prefabResource = Resource.Load(bunkerPrefab);
        if (!prefabResource || !prefabResource.IsValid())
        {
            Print("BunkerManager: Invalid or missing bunker prefab!", LogLevel.ERROR);
            return null;
        }

        World world = GetGame().GetWorld();
        if (!world)
        {
            Print("BunkerManager: World is null!", LogLevel.ERROR);
            return null;
        }

        EntitySpawnParams spawnParams = new EntitySpawnParams();
        spawnParams.Transform[3] = position;
        spawnParams.Transform[0] = Vector(1, 0, 0);
        spawnParams.Transform[1] = Vector(0, 1, 0);
        spawnParams.Transform[2] = Vector(0, 0, 1);

        IEntity spawnedEntity = GetGame().SpawnEntityPrefab(prefabResource, world, spawnParams);
        if (!spawnedEntity)
        {
            Print("BunkerManager: Failed to spawn bunker entity!", LogLevel.ERROR);
            return null;
        }

        EXTRACT_BunkerEntity bunker = EXTRACT_BunkerEntity.Cast(spawnedEntity);
        if (!bunker)
        {
            Print("BunkerManager: Failed to cast to EXTRACT_BunkerEntity!", LogLevel.ERROR);
            SCR_EntityHelper.DeleteEntityAndChildren(spawnedEntity);
            return null;
        }

        EPF_PersistenceComponent persistenceComp = EPF_PersistenceComponent.Cast(bunker.FindComponent(EPF_PersistenceComponent));
        if (!persistenceComp)
        {
            Print("BunkerManager: Bunker has no EPF_PersistenceComponent! Persistence will not work.", LogLevel.WARNING);
        }

        m_AllBunkers.Insert(bunker);
        return bunker;
    }
}

// Klasse zur Verwaltung von Loot
class EXTRACT_LootManager
{
    void EXTRACT_LootManager()
    {
    }

    bool SpawnLootInContainer(IEntity container)
    {
        if (!container)
        {
            return false;
        }

        Extraction_StorageEntity storage = Extraction_StorageEntity.Cast(container);
        if (!storage)
        {
            return false;
        }

        if (!storage.IsLootable())
        {
            return false;
        }

        ActionsManagerComponent actionsManager = ActionsManagerComponent.Cast(storage.FindComponent(ActionsManagerComponent));
        if (!actionsManager)
        {
            return false;
        }

        EXTRACT_LootStorageAction lootAction = EXTRACT_LootStorageAction.Cast(storage.FindComponent(EXTRACT_LootStorageAction));
        array<ref ResourceName> configs = null;
        array<int> configWeights = null;

        if (lootAction)
        {
            configs = lootAction.GetConfigs();
        }

        if (!lootAction || !configs || configs.IsEmpty())
        {
            configs = new array<ref ResourceName>();
            configWeights = new array<int>();

            configs.Insert("{73631F7704E52588}Configs/Loot/Medical.conf");            configWeights.Insert(300);
            configs.Insert("{2619506E4D9E9340}Configs/Loot/Military_Clothing.conf");  configWeights.Insert(250);
            configs.Insert("{10FB16FF4EC7FB38}Configs/Loot/Military_Misc.conf");     configWeights.Insert(15);
            configs.Insert("{89AC01820DDCE5C0}Configs/Loot/Military_Weapons.conf");   configWeights.Insert(15);
            configs.Insert("{3919DD91B8B1002B}Configs/Loot/Residential_Misc.conf");  configWeights.Insert(300);
            configs.Insert("{AD3FC8945B29C4C7}Configs/Loot/Residential_Weapons.conf"); configWeights.Insert(120);
        }

        if (!configs || configs.IsEmpty())
        {
            return false;
        }

        int itemCount = Math.RandomIntInclusive(0, 6);

        array<ref ResourceName> availableConfigs = new array<ref ResourceName>();
        array<int> availableWeights = new array<int>();
        foreach (int i, ResourceName configName : configs)
        {
            availableConfigs.Insert(configName);
            if (configWeights && i < configWeights.Count())
                availableWeights.Insert(configWeights[i]);
            else
                availableWeights.Insert(100 / configs.Count());
        }

        SCR_InventoryStorageManagerComponent storageManager = SCR_InventoryStorageManagerComponent.Cast(storage.FindComponent(SCR_InventoryStorageManagerComponent));
        if (!storageManager)
        {
            return false;
        }

        bool success = false;
        for (int i = 0; i < itemCount; i++)
        {
            ResourceName config;
            if (availableConfigs.IsEmpty())
            {
                break;
            }

            int totalWeight = 0;
            foreach (int weight : availableWeights)
            {
                totalWeight += weight;
            }
            if (totalWeight > 0)
            {
                int randomValue = Math.RandomInt(0, totalWeight);
                int currentSum = 0;
                for (int j = 0; j < availableConfigs.Count(); j++)
                {
                    currentSum += availableWeights[j];
                    if (randomValue < currentSum)
                    {
                        config = availableConfigs[j];
                        availableConfigs.Remove(j);
                        availableWeights.Remove(j);
                        break;
                    }
                }
            }
            else
            {
                int randomIndex = Math.RandomIntInclusive(0, availableConfigs.Count() - 1);
                config = availableConfigs[randomIndex];
                availableConfigs.Remove(randomIndex);
                availableWeights.Remove(randomIndex);
            }

            if (!config)
            {
                continue;
            }

            Resource configResource = Resource.Load(config);
            if (!configResource || !configResource.IsValid())
            {
                continue;
            }

            BaseContainer configContainer = configResource.GetResource().ToBaseContainer();
            if (!configContainer)
            {
                continue;
            }

            ZEL_TieredLootConfig configInstance = ZEL_TieredLootConfig.Cast(BaseContainerTools.CreateInstanceFromContainer(configContainer));
            if (!configInstance)
            {
                continue;
            }

            array<string> lootPrefabs = {};
            array<int> probabilities = {};
            int totalProbability = 0;

            if (configInstance.m_TierData && !configInstance.m_TierData.IsEmpty())
            {
                foreach (ZEL_TierData item : configInstance.m_TierData)
                {
                    if (!item.m_sPrefab)
                    {
                        continue;
                    }
                    lootPrefabs.Insert(item.m_sPrefab);
                    int probability = item.m_iProbability;
                    if (probability <= 0)
                        probability = 1;
                    probabilities.Insert(probability);
                    totalProbability += probability;
                }
            }

            if (lootPrefabs.IsEmpty())
            {
                continue;
            }

            string selectedPrefab = "";
            if (totalProbability > 0)
            {
                int randomValue = Math.RandomInt(0, totalProbability);
                int currentSum = 0;
                for (int j = 0; j < lootPrefabs.Count(); j++)
                {
                    currentSum += probabilities[j];
                    if (randomValue < currentSum)
                    {
                        selectedPrefab = lootPrefabs[j];
                        break;
                    }
                }
            }
            else
            {
                selectedPrefab = lootPrefabs.GetRandomElement();
            }

            if (!selectedPrefab)
            {
                continue;
            }

            Resource prefabResource = Resource.Load(selectedPrefab);
            if (!prefabResource || !prefabResource.IsValid())
            {
                continue;
            }

            World world = GetGame().GetWorld();
            if (!world)
            {
                return false;
            }

            IEntity item = GetGame().SpawnEntityPrefab(prefabResource, world, null);
            if (!item)
            {
                continue;
            }

            if (!storageManager.TryInsertItem(item))
            {
                SCR_EntityHelper.DeleteEntityAndChildren(item);
                continue;
            }

            success = true;
        }

        if (!success)
        {
            return false;
        }

        return true;
    }

    void Unload()
    {
    }
}

// Datenklasse für die Persistenz von Bunkern
class EXTRACT_BunkerSaveData : EDF_DbEntity
{
    int m_iPlayerId;
    string m_sPlayerUuid;
    vector m_vPosition;

    string GetPersistentId()
    {
        return m_sPlayerUuid;
    }
}

bool Init()
{
    Print("scripts.c: Init() wurde aufgerufen", LogLevel.NORMAL);
    if (g_ModuleInitialized)
    {
        Print("scripts.c: Init() wurde bereits ausgeführt", LogLevel.WARNING);
        return true;
    }

    g_ModuleInitialized = true;
    GetGame().GetCallqueue().CallLater(InitModuleSafely, 100, false);
    return true;
}

void InitModuleSafely()
{
    Print("scripts.c: InitModuleSafely() wird ausgeführt", LogLevel.NORMAL);

    EXTRACT_ServerModule mod = EXTRACT_ServerModule.GetInstance();
    if (mod)
    {
        Print("scripts.c: ServerModule wurde erfolgreich initialisiert", LogLevel.NORMAL);
        mod.OnGameStart();
    }
    else
    {
        Print("scripts.c: Fehler – ServerModule ist null", LogLevel.ERROR);
    }

    g_BunkerManager = new BunkerManager();
    if (g_BunkerManager)
    {
        Print("scripts.c: BunkerManager erfolgreich erstellt.", LogLevel.NORMAL);
    }
    else
    {
        Print("scripts.c: Fehler – BunkerManager konnte nicht erstellt werden!", LogLevel.ERROR);
    }

    g_LootManager = new EXTRACT_LootManager();
    if (g_LootManager)
    {
        Print("scripts.c: EXTRACT_LootManager erfolgreich erstellt.", LogLevel.NORMAL);
    }
    else
    {
        Print("scripts.c: Fehler – EXTRACT_LootManager konnte nicht erstellt werden!", LogLevel.ERROR);
    }
}

void OnGameStart()
{
    if (g_BunkerManager)
        g_BunkerManager.OnGameStart();
}

void OnPlayerDisconnected(int playerId, KickCauseCode cause, int timeout)
{
    if (g_BunkerManager)
        g_BunkerManager.OnPlayerDisconnected(playerId, cause, timeout);
}

void OnPlayerAuditFail(int playerId)
{
    if (g_BunkerManager)
        g_BunkerManager.OnPlayerAuditFail(playerId);
}

void OnPlayerKilled(int playerId, IEntity playerEntity, IEntity killerEntity, Instigator killer)
{
    if (g_BunkerManager)
        g_BunkerManager.OnPlayerKilled(playerId, playerEntity, killerEntity, killer);
}

IEntity GetBunker(int playerId)
{
    if (g_BunkerManager)
        return g_BunkerManager.GetBunker(playerId);
    return null;
}

IEntity CreateSingleBunkerForPlayer(int playerId)
{
    if (g_BunkerManager)
        return g_BunkerManager.CreateSingleBunkerForPlayer(playerId);
    return null;
}

IEntity CreateBunker()
{
    if (g_BunkerManager)
        return g_BunkerManager.CreateBunker();
    return null;
}

bool RemoveBunker(IEntity bunkerEntity)
{
    if (g_BunkerManager)
        return g_BunkerManager.RemoveBunker(EXTRACT_BunkerEntity.Cast(bunkerEntity));
    return false;
}

bool RemoveBunkerForPlayer(int playerId)
{
    if (g_BunkerManager)
        return g_BunkerManager.RemoveBunkerForPlayer(playerId);
    return false;
}

void RegisterPlayerCharacter(SCR_ChimeraCharacter character)
{
    if (g_BunkerManager)
        g_BunkerManager.RegisterPlayerCharacter(character);
}

void UnregisterPlayerCharacter(SCR_ChimeraCharacter character)
{
    if (g_BunkerManager)
        g_BunkerManager.UnregisterPlayerCharacter(character);
}

void PlayerTeleported(SCR_ChimeraCharacter character)
{
    if (g_BunkerManager)
        g_BunkerManager.PlayerTeleported(character);
}

void Unload()
{
    if (g_BunkerManager)
    {
        g_BunkerManager.Unload();
        g_BunkerManager = null;
    }
    if (g_LootManager)
    {
        g_LootManager.Unload();
        g_LootManager = null;
    }
}

bool SpawnLootInContainer(IEntity container)
{
    if (g_LootManager)
        return g_LootManager.SpawnLootInContainer(container);
    Print("scripts.c: SpawnLootInContainer: g_LootManager is null!", LogLevel.ERROR);
    return false;
}